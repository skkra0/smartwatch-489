#define DEBUG (1)
#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) DEBUG_PRINT(x); Serial.println();
#else
  #define DEBUG_PRINT(x) // do nothing
  #define DEBUG_PRINTLN(x)
#endif