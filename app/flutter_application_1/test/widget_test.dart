import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:flutter_application_1/main.dart';

void main() {
  testWidgets('Launch page appears on startup', (WidgetTester tester) async {
    
    await tester.pumpWidget(const MyApp());

    
    expect(find.text('Vital Logger'), findsOneWidget);
    expect(find.byIcon(Icons.favorite), findsOneWidget);
  });

  testWidgets('Navigate to home page after launch screen',
      (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    
    await tester.pumpAndSettle(const Duration(seconds: 4));

    
    expect(find.text('Login'), findsWidgets);
    expect(find.text('Sign Up'), findsWidgets);
  });
}
