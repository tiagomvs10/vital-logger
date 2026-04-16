import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';

class AuthProcessPage extends StatefulWidget {
  final String userId;

  const AuthProcessPage({super.key, required this.userId});

  @override
  State<AuthProcessPage> createState() => _AuthProcessPageState();
}

class _AuthProcessPageState extends State<AuthProcessPage> {
  String? profilePhotoUrl;
  String? lastPhotoUrl;
  String? authStatus;
  bool isProcessing = true;
  late int pageLoadTime;

  @override
  void initState() {
    super.initState();
    pageLoadTime = DateTime.now().millisecondsSinceEpoch;
    _initializeAuthProcess();
  }

  Future<void> _initializeAuthProcess() async {
    try {
      
      final DatabaseReference profileRef = 
          FirebaseDatabase.instance.ref('users/${widget.userId}/profile_photo_url');
      final profileSnapshot = await profileRef.get();
      if (profileSnapshot.exists) {
        if (mounted) {
          setState(() {
            profilePhotoUrl = profileSnapshot.value.toString();
          });
        }
      }

      
      final DatabaseReference sessionRef = 
          FirebaseDatabase.instance.ref('users/${widget.userId}/current_session');
      
      sessionRef.onValue.listen((DatabaseEvent event) {
        if (event.snapshot.value != null) {
          final Map<dynamic, dynamic> data = 
              event.snapshot.value as Map<dynamic, dynamic>;
          
          if (mounted) {
            setState(() {
              
              if (data.containsKey('last_photo')) {
                final String photoUrl = data['last_photo'].toString();
                final int timestamp = DateTime.now().millisecondsSinceEpoch;
                lastPhotoUrl = '$photoUrl&t=$timestamp';
              }
              
              
              if (data.containsKey('auth_status')) {
                authStatus = data['auth_status'];
                if (authStatus != null) {
                  isProcessing = false;
                }
              }
            });
          }
        }
      });
    } catch (e) {
      if (mounted) {
        setState(() {
          isProcessing = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Authentication',
          style: TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.w900,
          ),
        ),
        backgroundColor: const Color(0xFF00BFFF),
        foregroundColor: Colors.white,
        toolbarHeight: 46,
      ),
      body: Column(
        children: [
          
          Expanded(
            flex: 1,
            child: Container(
              width: double.infinity,
              color: Colors.grey[300],
              child: profilePhotoUrl != null
                  ? Image.network(
                      profilePhotoUrl!,
                      fit: BoxFit.cover,
                      errorBuilder: (context, error, stackTrace) {
                        return const Center(
                          child: Icon(Icons.person, size: 80),
                        );
                      },
                    )
                  : Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const CircularProgressIndicator(),
                          const SizedBox(height: 12),
                          Text(
                            'Loading profile...',
                            style: TextStyle(
                              fontSize: 14,
                              color: Colors.grey[700],
                            ),
                          ),
                        ],
                      ),
                    ),
            ),
          ),
          
          const Divider(height: 2, color: Colors.white),
          
          Expanded(
            flex: 1,
            child: Container(
              width: double.infinity,
              color: Colors.black,
              child: lastPhotoUrl == null
                  ? Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const CircularProgressIndicator(
                            valueColor:
                                AlwaysStoppedAnimation<Color>(Colors.white),
                          ),
                          const SizedBox(height: 12),
                          const Text(
                            'Waiting for camera...',
                            style: TextStyle(
                              fontSize: 14,
                              color: Colors.white,
                            ),
                          ),
                        ],
                      ),
                    )
                  : Image.network(
                      lastPhotoUrl!,
                      fit: BoxFit.cover,
                      errorBuilder: (context, error, stackTrace) {
                        return const Center(
                          child: Icon(
                            Icons.camera_alt,
                            size: 80,
                            color: Colors.grey,
                          ),
                        );
                      },
                    ),
            ),
          ),
          
          SafeArea(
            top: false,
            child: Container(
              width: double.infinity,
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.white,
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withOpacity(0.2),
                    blurRadius: 8,
                    offset: const Offset(0, -2),
                  ),
                ],
              ),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  
                  if (authStatus == null && isProcessing)
                    Column(
                      children: [
                        const CircularProgressIndicator(
                          valueColor:
                              AlwaysStoppedAnimation<Color>(Colors.deepPurple),
                        ),
                        const SizedBox(height: 12),
                        const Text(
                          'Processing...',
                          style: TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                            color: Colors.black,
                          ),
                        ),
                      ],
                    )
                  else if (authStatus == 'authenticated')
                    Column(
                      children: [
                        const Icon(
                          Icons.check_circle,
                          color: Colors.green,
                          size: 48,
                        ),
                        const SizedBox(height: 12),
                        const Text(
                          'Identity Confirmed',
                          style: TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w900,
                            color: Colors.green,
                          ),
                        ),
                      ],
                    )
                  else if (authStatus == 'denied')
                    Column(
                      children: [
                        const Icon(
                          Icons.cancel,
                          color: Colors.red,
                          size: 48,
                        ),
                        const SizedBox(height: 12),
                        const Text(
                          'Access Denied',
                          style: TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w900,
                            color: Colors.red,
                          ),
                        ),
                      ],
                    ),
                  const SizedBox(height: 16),
                  
                  SizedBox(
                    width: double.infinity,
                    height: 46,
                    child: ElevatedButton(
                      onPressed: isProcessing ? null : () => Navigator.pop(context),
                      style: ElevatedButton.styleFrom(
                        backgroundColor:
                            isProcessing ? Colors.grey : Colors.deepPurple,
                        foregroundColor: Colors.white,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                      child: const Text(
                        'Close',
                        style: TextStyle(
                          fontSize: 15,
                          fontWeight: FontWeight.w900,
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

