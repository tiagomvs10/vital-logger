import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:image_picker/image_picker.dart';
import 'dart:typed_data';

class AuthService {
  final FirebaseAuth _auth = FirebaseAuth.instance;
  final FirebaseDatabase _database = FirebaseDatabase.instance;

  
  Future<String?> signUp({
    required String email,
    required String password,
    required String name,
    required int age,
    required XFile profileImage,
  }) async {
    try {
      
      final UserCredential userCredential = await _auth.createUserWithEmailAndPassword(
        email: email,
        password: password,
      );

      final String uid = userCredential.user!.uid;

      try {
      
        final Uint8List fileBytes = await profileImage.readAsBytes();

        
        final storageRef = FirebaseStorage.instance
            .ref()
            .child("profilePics")
            .child("$uid.jpg");

        final SettableMetadata metadata = SettableMetadata(
          contentType: 'image/jpeg',
        );

        await storageRef.putData(fileBytes, metadata);

        
        final String profilePhotoUrl = await storageRef.getDownloadURL();

        
        await _database.ref('users/$uid').set({
          'name': name,
          'age': age,
          'email': email,
          'profile_photo_url': profilePhotoUrl,
        });

        
        await _auth.currentUser?.reload();

        return null; 
      } catch (uploadError) {
        
        try {
          await userCredential.user?.delete();
        } catch (deleteError) {
          
          print('Error deleting user after failed upload: $deleteError');
        }
        return 'Error saving profile: ${uploadError.toString()}';
      }
    } on FirebaseAuthException catch (e) {
      return e.message ?? 'An error occurred during sign up';
    } catch (e) {
      return 'An unexpected error occurred: ${e.toString()}';
    }
  }

  
  Future<String?> signIn({
    required String email,
    required String password,
  }) async {
    try {
      await _auth.signInWithEmailAndPassword(
        email: email,
        password: password,
      );
      return null; 
    } on FirebaseAuthException catch (e) {
      return e.message ?? 'An error occurred during sign in';
    } catch (e) {
      return 'An unexpected error occurred: ${e.toString()}';
    }
  }

  
  Future<void> signOut() async {
    try {
      final User? currentUser = _auth.currentUser;
      if (currentUser != null) {
        final String uid = currentUser.uid;
        
        await _database.ref('users/$uid/current_session').remove();
      }
      await _auth.signOut();
    } catch (e) {
      
      print('Error signing out: $e');
    }
  }

  
  User? getCurrentUser() {
    return _auth.currentUser;
  }

  
  Future<Map<String, dynamic>?> getUserData(String userId) async {
    try {
      final snapshot = await _database.ref('users/$userId').get();
      if (snapshot.exists) {
        return Map<String, dynamic>.from(snapshot.value as Map);
      }
      return null;
    } catch (e) {
      print('Error fetching user data: $e');
      return null;
    }
  }

  
  Future<String?> uploadProfilePhoto(XFile imageFile, String userId) async {
    try {
      
      final storageRef = FirebaseStorage.instance
          .ref()
          .child("profilePics")
          .child("$userId.jpg");

      
      final Uint8List fileBytes = await imageFile.readAsBytes();
      await storageRef.putData(fileBytes);

      
      String downloadUrl = await storageRef.getDownloadURL();

      
      DatabaseReference userRef = FirebaseDatabase.instance.ref("users/$userId");
      await userRef.update({
        "profile_photo_url": downloadUrl
      });

      return null; 
    } catch (e) {
      return "Error uploading profile photo: ${e.toString()}";
    }
  }

  
  Future<String?> deleteProfile(String userId, String password) async {
    try {
      final currentUser = _auth.currentUser;
      if (currentUser == null || currentUser.email == null) {
        return 'No user is currently logged in';
      }

      
      try {
        final credential = EmailAuthProvider.credential(
          email: currentUser.email!,
          password: password,
        );
        await currentUser.reauthenticateWithCredential(credential);
      } on FirebaseAuthException catch (e) {
        if (e.code == 'wrong-password') {
          return 'Incorrect password. Please try again.';
        }
        return 'Authentication failed: ${e.message}';
      }

      
      try {
        final storageRef = FirebaseStorage.instance
            .ref()
            .child("profilePics")
            .child("$userId.jpg");
        await storageRef.delete();
      } catch (e) {
        
        print('Note: Could not delete photo: $e');
      }

      
      try {
        await _database.ref('users/$userId').remove();
        await _database.ref('measurements/$userId').remove();
      } catch (e) {
        print('Error deleting database entries: $e');
      }

      
      try {
        await currentUser.delete();
        print('Account deleted successfully');
      } catch (e) {
        print('Error deleting account: $e');
        
      }

      
      try {
        await _auth.signOut();
        print('User signed out successfully');
      } catch (e) {
        print('Error signing out: $e');
      }

      return null; // Success
    } on FirebaseAuthException catch (e) {
      return 'Error: ${e.message}';
    } catch (e) {
      return 'Error deleting profile: ${e.toString()}';
    }
  }

  
  Future<String?> updateProfile({
    required String uid,
    String? name,
    int? age,
    String? newPassword,
    XFile? newImage,
  }) async {
    try {
      
      if (name != null || age != null) {
        final updates = <String, dynamic>{};
        if (name != null) updates['name'] = name;
        if (age != null) updates['age'] = age;
        
        await _database.ref('users/$uid').update(updates);
      }

      
      if (newPassword != null && newPassword.isNotEmpty) {
        try {
          await _auth.currentUser?.updatePassword(newPassword);
        } on FirebaseAuthException catch (e) {
          if (e.code == 'requires-recent-login') {
            return 'Password change requires recent login. Please sign out and sign in again to change your password.';
          }
          return 'Error updating password: ${e.message}';
        }
      }

      
      if (newImage != null) {
        try {
          
          final Uint8List fileBytes = await newImage.readAsBytes();

          
          final storageRef = FirebaseStorage.instance
              .ref()
              .child("profilePics")
              .child("$uid.jpg");

          final SettableMetadata metadata = SettableMetadata(
            contentType: 'image/jpeg',
          );

          await storageRef.putData(fileBytes, metadata);

          
          final String profilePhotoUrl = await storageRef.getDownloadURL();
          await _database.ref('users/$uid').update({
            'profile_photo_url': profilePhotoUrl,
          });
        } catch (imageError) {
          return 'Error updating profile image: ${imageError.toString()}';
        }
      }

      return null; 
    } catch (e) {
      return 'Error updating profile: ${e.toString()}';
    }
  }
}

