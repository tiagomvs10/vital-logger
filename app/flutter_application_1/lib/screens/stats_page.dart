import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:intl/intl.dart';

class StatsPage extends StatefulWidget {
  const StatsPage({super.key});

  @override
  State<StatsPage> createState() => _StatsPageState();
}

class _StatsPageState extends State<StatsPage> {
  List<FlSpot> bpmSpots = [];
  List<FlSpot> spo2Spots = [];
  Map<int, String> dayLabels = {};
  bool isLoading = true;
  int _selectedRange = 30;
  List<MapEntry<dynamic, dynamic>> _rawData = [];

  @override
  void initState() {
    super.initState();
    _fetchMeasurements();
  }

  Future<void> _fetchMeasurements() async {
    try {
      final String? uid = FirebaseAuth.instance.currentUser?.uid;
      if (uid == null) return;

      final DatabaseReference ref =
          FirebaseDatabase.instance.ref('measurements/$uid');
      final snapshot = await ref.get();

      if (snapshot.exists) {
        final Map<dynamic, dynamic> data =
            snapshot.value as Map<dynamic, dynamic>;

        
        _rawData = data.entries.toList()
          ..sort((a, b) {
            final tA = a.value['timestamp'] ?? 0;
            final tB = b.value['timestamp'] ?? 0;
            return tA.compareTo(tB);
          });

        
        _filterData();

        if (mounted) {
          setState(() {
            isLoading = false;
          });
        }
      } else {
        if (mounted) setState(() => isLoading = false);
      }
    } catch (e) {
      debugPrint("Error fetching stats: $e");
      if (mounted) setState(() => isLoading = false);
    }
  }

  double _calculateMinY() {
    if (bpmSpots.isEmpty && spo2Spots.isEmpty) return 0;
    
    double minValue = 180;
    for (final spot in bpmSpots) {
      if (spot.y < minValue) minValue = spot.y;
    }
    for (final spot in spo2Spots) {
      if (spot.y < minValue) minValue = spot.y;
    }
    
    
    return (minValue - 10).clamp(0, 180);
  }

  double _calculateMaxY() {
    if (bpmSpots.isEmpty && spo2Spots.isEmpty) return 100;
    
    double maxValue = 0;
    for (final spot in bpmSpots) {
      if (spot.y > maxValue) maxValue = spot.y;
    }
    for (final spot in spo2Spots) {
      if (spot.y > maxValue) maxValue = spot.y;
    }
    
    
    return maxValue + 10;
  }

  void _filterData() {
    bpmSpots = [];
    spo2Spots = [];
    dayLabels = {};

    
    final DateTime now = DateTime.now();
    final DateTime cutoffDate = now.subtract(Duration(days: _selectedRange));
    final int cutoffMs = cutoffDate.millisecondsSinceEpoch;

    
    for (var entry in _rawData) {
      if (entry.key == 'live') continue;

      final val = entry.value;
      
      final int timestampSecs = val['timestamp'] is int
          ? val['timestamp']
          : int.tryParse(val['timestamp'].toString()) ?? 0;
      final int timestampMs = timestampSecs * 1000;

      
      if (timestampMs > cutoffMs) {
        final double bpm = (val['bpm'] ?? 0).toDouble();
        final double spo2 = (val['spo2'] ?? 0).toDouble();

        bpmSpots.add(FlSpot(timestampMs.toDouble(), bpm));
        spo2Spots.add(FlSpot(timestampMs.toDouble(), spo2));

        
        final dt = DateTime.fromMillisecondsSinceEpoch(timestampMs);
        dayLabels[timestampMs] = DateFormat('dd/MM').format(dt);
      }
    }

    if (mounted) {
      setState(() {});
    }
  }

  Widget _buildFilterButton(int days, String label) {
    final isSelected = _selectedRange == days;
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: isSelected ? Colors.deepPurple : Colors.grey[300],
        foregroundColor: isSelected ? Colors.white : Colors.black,
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      ),
      onPressed: () {
        setState(() {
          _selectedRange = days;
        });
        _filterData();
      },
      child: Text(label),
    );
  }

  @override
  Widget build(BuildContext context) {
    
    double xInterval = 1.0;
    if (bpmSpots.isNotEmpty) {
      final double minX = bpmSpots.first.x;
      final double maxX = bpmSpots.last.x;
      final double xRange = maxX - minX;
      
      if (xRange > 0) {
        
        xInterval = _selectedRange == 1 ? xRange / 2.0 : xRange / 3.0;
      }
    }
    
    
    if (xInterval == 0) xInterval = 1.0;

    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Statistics',
          style: TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.w900,
          ),
        ),
        backgroundColor: const Color(0xFF00BFFF),
        foregroundColor: Colors.white,
        toolbarHeight: 46,
      ),
      body: isLoading
          ? const Center(child: CircularProgressIndicator())
          : bpmSpots.isEmpty
              ? const Center(
                  child: Text('No measurements data available'),
                )
              : Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    children: [
                      Text(
                        'Last $_selectedRange Days',
                        style: const TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.w900,
                          color: Colors.black,
                        ),
                      ),
                      const SizedBox(height: 12),
                      
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                        children: [
                          _buildFilterButton(1, '1 Day'),
                          _buildFilterButton(7, '7 Days'),
                          _buildFilterButton(30, '30 Days'),
                        ],
                      ),
                      const SizedBox(height: 16),
                      const SizedBox(height: 20),
                      Expanded(
                        child: LineChart(
                          LineChartData(
                            gridData: FlGridData(
                              show: true,
                              drawVerticalLine: false,
                              horizontalInterval: 20,
                              getDrawingHorizontalLine: (value) {
                                return FlLine(
                                  color: Colors.grey[200],
                                  strokeWidth: 0.5,
                                );
                              },
                            ),
                            titlesData: FlTitlesData(
                              show: true,
                              bottomTitles: AxisTitles(
                                sideTitles: SideTitles(
                                  showTitles: true,
                                  reservedSize: 30,
                                  
                                  interval: xInterval,
                                  getTitlesWidget: (value, meta) {
                                    final int timestamp = value.toInt();
                                    final dt = DateTime.fromMillisecondsSinceEpoch(timestamp);

                                    
                                    final String dateStr = _selectedRange == 1
                                        ? DateFormat('HH:mm').format(dt)
                                        : DateFormat('dd/MM').format(dt);

                                    return SideTitleWidget(
                                      meta: meta,
                                      space: 8,
                                      fitInside: SideTitleFitInsideData.fromTitleMeta(meta),
                                      child: Text(
                                        dateStr,
                                        style: const TextStyle(
                                          color: Colors.black,
                                          fontWeight: FontWeight.w600,
                                          fontSize: 10,
                                        ),
                                      ),
                                    );
                                  },
                                ),
                              ),
                              leftTitles: AxisTitles(
                                sideTitles: SideTitles(
                                  showTitles: true,
                                  reservedSize: 40,
                                  getTitlesWidget: (value, meta) {
                                    return Text(
                                      value.toInt().toString(),
                                      style: const TextStyle(
                                        color: Colors.black,
                                        fontWeight: FontWeight.w600,
                                        fontSize: 12,
                                      ),
                                    );
                                  },
                                ),
                              ),
                              topTitles: const AxisTitles(
                                sideTitles: SideTitles(showTitles: false),
                              ),
                              rightTitles: const AxisTitles(
                                sideTitles: SideTitles(showTitles: false),
                              ),
                            ),
                            borderData: FlBorderData(show: false),
                            lineTouchData: LineTouchData(
                              enabled: true,
                            ),
                            minX: bpmSpots.isNotEmpty
                                ? bpmSpots.first.x
                                : 0,
                            maxX: bpmSpots.isNotEmpty
                                ? bpmSpots.last.x
                                : 10,
                            minY: _calculateMinY(),
                            maxY: _calculateMaxY(),
                            lineBarsData: [
                              
                              LineChartBarData(
                                spots: bpmSpots,
                                isCurved: true,
                                color: Colors.red,
                                barWidth: 3,
                                isStrokeCapRound: true,
                                dotData: const FlDotData(show: false),
                                belowBarData: BarAreaData(
                                  show: true,
                                  gradient: LinearGradient(
                                    begin: Alignment.topCenter,
                                    end: Alignment.bottomCenter,
                                    colors: [
                                      Colors.red.withOpacity(0.3),
                                      Colors.red.withOpacity(0.0),
                                    ],
                                  ),
                                ),
                              ),
                              
                              LineChartBarData(
                                spots: spo2Spots,
                                isCurved: true,
                                color: const Color(0xFF00BFFF),
                                barWidth: 4,
                                isStrokeCapRound: true,
                                dotData: const FlDotData(show: false),
                                belowBarData: BarAreaData(
                                  show: true,
                                  gradient: LinearGradient(
                                    begin: Alignment.topCenter,
                                    end: Alignment.bottomCenter,
                                    colors: [
                                      const Color(0xFF00BFFF).withOpacity(0.2),
                                      const Color(0xFF00BFFF).withOpacity(0.0),
                                    ],
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                      const SizedBox(height: 24),
                      
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Row(
                            children: [
                              Container(
                                width: 12,
                                height: 12,
                                decoration: BoxDecoration(
                                  color: Colors.red,
                                  shape: BoxShape.circle,
                                ),
                              ),
                              const SizedBox(width: 8),
                              const Text(
                                'Heart Rate (bpm)',
                                style: TextStyle(
                                  fontSize: 12,
                                  fontWeight: FontWeight.w600,
                                  color: Colors.black,
                                ),
                              ),
                            ],
                          ),
                          const SizedBox(width: 24),
                          Row(
                            children: [
                              Container(
                                width: 12,
                                height: 12,
                                decoration: BoxDecoration(
                                  color: const Color(0xFF00BFFF),
                                  shape: BoxShape.circle,
                                ),
                              ),
                              const SizedBox(width: 8),
                              const Text(
                                'SpO2 (%)',
                                style: TextStyle(
                                  fontSize: 12,
                                  fontWeight: FontWeight.w600,
                                  color: Colors.black,
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
    );
  }
}
