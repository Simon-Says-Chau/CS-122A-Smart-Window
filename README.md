# CS-122A-Smart-Window

I built an automatic/manual curtain roller that is determined by the amount of sunlight it detects.  For sunlight simulation, I used IR sensors and blocked them to simulate darkness.  The curtains/blinds should automatically adjust according to whether the IR sensor has a high value or a low value.  If the value is high, that simulates sunlight, and the curtains/blinds should rise to allow light into the house.  If the value is low, that simulate darkness, and the curtains/blinds should drop so people do not see into the house easily.  If set to become manual, the curtains/blinds will not move automatically and can be manually adjusted via keypad.  This project is in hopes of helping to save electricity by automatically providing natural light.

User Guide
-	Connect Phone to Bluetooth Module using App
-	“Auto” Button allows the system to run automatically
-	“Manual” Button allows the user to have manual control over the system
-	If the IR sensors are exposed to each other while the system is in Auto mode, then the curtains/blinds will rise on their own
-	If the IR sensors are blocked from each other while the system is in Auto mode, then the curtains/blinds will drop on their own
-	If the system is in Manual mode, blocking or exposing the IR sensors to each other will have no effect
-	In manual mode, the User can press C to move the curtains/blinds up or B to move the curtains/blinds down
