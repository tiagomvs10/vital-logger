import 'package:firebase_core/firebase_core.dart' show FirebaseOptions;
import 'package:flutter/foundation.dart' show kIsWeb;

class DefaultFirebaseOptions {
  static FirebaseOptions get currentPlatform {
    if (kIsWeb) {
      return web;
    }
    throw UnsupportedError(
      'DefaultFirebaseOptions are not supported for this platform.',
    );
  }

  static const FirebaseOptions web = FirebaseOptions(
    apiKey: 'AIzaSyBmNphplSwcgouqgMXuNTjyMrKiVGiP6zI',
    appId: '1:550817088118:web:9df66e32381f71f6d5d557',
    messagingSenderId: '550817088118',
    projectId: 'vitallogger-ae687',
    storageBucket: 'vitallogger-ae687.firebasestorage.app',
    databaseURL: 'https://vitallogger-ae687-default-rtdb.firebaseio.com',
  );
}
