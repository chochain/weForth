///
/// eForth - Configuration and Cross Platform macros
///
#ifndef __EFORTH_SRC_CONFIG_H
#define __EFORTH_SRC_CONFIG_H
///
/// Benchmark: 10K*10K cycles on desktop (3.2G AMD)
///    RANGE_CHECK     0 cut 100ms
///    INLINE            cut 545ms
///
///@name Conditional compililation options
///@}
#define APP_VERSION     "weForth v4.2"
#define CC_DEBUG        1               /**< debug level 0|1|2      */
#define RANGE_CHECK     0               /**< vector range check     */
#define USE_FLOAT       1               /**< support floating point */
#define CASE_SENSITIVE  1               /**< word case sensitivity  */
#define DO_WASM         __EMSCRIPTEN__  /**< for WASM output        */
///@}
///@name Memory block configuation
///@{
#define E4_RS_SZ        32
#define E4_SS_SZ        32
#define E4_DICT_SZ      400
#define E4_PMEM_SZ      (32*1024)
///@}
///
///@name Logical units (instead of physical) for type check and portability
///@{
typedef uint32_t        U32;   ///< unsigned 32-bit integer
typedef int32_t         S32;   ///< signed 32-bit integer
typedef uint16_t        U16;   ///< unsigned 16-bit integer
typedef uint8_t         U8;    ///< byte, unsigned character

typedef uintptr_t       UFP;   ///< function pointer as integer
typedef uint16_t        IU;    ///< instruction pointer unit

#if USE_FLOAT
#include <cmath>
typedef double          DU2;
typedef float           DU;
#define DU0             0.0f
#define DU1             1.0f
#define DU_EPS          0.00001f
#define UINT(v)         (static_cast<U32>(v))
#define MOD(m,n)        (fmodf(m,n))
#define ABS(v)          (fabsf(v))
#define ZEQ(v)          (ABS(v) < DU_EPS)
#define EQ(a,b)         (ZEQ((a) - (b)))
#define LT(a,b)         (((a) - (b)) < -DU_EPS)
#define GT(a,b)         (((a) - (b)) > DU_EPS)
#define RND()           (static_cast<float>(rand()) / static_cast<float>(RAND_MAX))

#else // !USE_FLOAT
typedef int64_t         DU2;
typedef int32_t         DU;
#define DU0             0
#define DU1             1
#define DU_EPS          0
#define UINT(v)         (static_cast<U32>(v))
#define MOD(m,n)        ((m) % (n))
#define ABS(v)          (abs(v))
#define ZEQ(v)          ((v)==DU0)
#define EQ(a,b)         ((a)==(b))
#define LT(a,b)         ((a) < (b))
#define GT(a,b)         ((a) > (b))
#define RND()           (rand())

#endif // USE_FLOAT

#if CASE_SENSITIVE
#define STRCMP(a,b)     strcmp(a,b)
#else  // !CASE_SENSITIVE
#include <strings.h>
#define STRCMP(a,b)     strcasecmp(a,b)
#endif // CASE_SENSITIVE
///@}
///@name Inline & Alignment macros
///@{
#define INLINE          __attribute__((always_inline))
#define ALIGN2(sz)      ((sz) + (-(sz) & 0x1))
#define ALIGN4(sz)      ((sz) + (-(sz) & 0x3))
#define ALIGN16(sz)     ((sz) + (-(sz) & 0xf))
#define ALIGN32(sz)     ((sz) + (-(sz) & 0x1f))
#define ALIGN(sz)       ALIGN2(sz)
#define STRLEN(s)       (ALIGN(strlen(s)+1))  /** calculate string size with alignment */
///@}
///@name Multi-platform support
///@{
#if    _WIN32 || _WIN64
    #define ENDL "\r\n"
#else  // !(_WIN32 || _WIN64)
    #define ENDL endl; fout_cb(fout.str().length(), fout.str().c_str()); fout.str("")
#endif // _WIN32 || _WIN64

#if (ARDUINO || ESP32)
    #include <Arduino.h>
    #define DALIGN(sz)      (sz)
    #define to_string(i)    string(String(i).c_str())
    #if    ESP32
        #define analogWrite(c,v,mx) ledcWrite((c),(8191/mx)*min((int)(v),mx))
    #endif // ESP32

#elif  DO_WASM
    #include <emscripten.h>
    #define DALIGN(sz)      ALIGN4(sz)
    #define millis()        EM_ASM_INT({ return Date.now(); })
    #define delay(ms)       EM_ASM({                                      \
                                const xhr = new XMLHttpRequest();         \
                                xhr.timeout = 1.1*$0;                     \
                                xhr.open('GET', "/SLEEP?t="+$0, false);   \
                                try { xhr.send(); } catch(e) {}           \
                            }, ms)
/*    
    #define delay(ms)       EM_ASM({                                      \
                                const t1 = Date.now() + $0;               \
                                while(Date.now() < t1);                   \
                            }, ms)
    #define delay(ms)       EM_ASM({                                      \
                                const b = wa.memory.buffer;               \
                                const a = new Int32Array(b);              \
                                a[0] = 0;                                 \
                                Atomics.wait(a,0,0,$0);                   \
                                }, ms)
    #define delay(ms)       emscripten_sleep_with_yield(ms)
*/
    #define yield()         /* JS is async */

#else  // !(ARDUINO || ESP32) && !DO_WASM
    #include <chrono>
    #include <thread>
    #define DALIGN(sz)      (sz)
    #define millis()        chrono::duration_cast<chrono::milliseconds>( \
                            chrono::steady_clock::now().time_since_epoch()).count()
    #define delay(ms)       this_thread::sleep_for(chrono::milliseconds(ms))
    #define yield()         this_thread::yield()
    #define PROGMEM

#endif // (ARDUINO || ESP32)
///@}
///@name Logging support
///@{
#if (ARDUINO || ESP32)
    #define LOGS(s)     Serial.print(F(s))
    #define LOG(v)      Serial.print(v)
    #define LOGX(v)     Serial.print(v, HEX)
#else  // !(ARDUINO || ESP32)
    #define LOGS(s)     printf("%s", s)
    #define LOG(v)      printf("%-d", (int32_t)(v))
    #define LOGX(v)     printf("%-x", (uint32_t)(v))
#endif // (ARDUINO || ESP32)
    
#define LOG_NA()        LOGS("N/A\n")
#define LOG_KV(k, v)    LOGS(k); LOG(v)
#define LOG_KX(k, x)    LOGS(k); LOGX(x)
#define LOG_HDR(f, s)   LOGS(f); LOGS("("); LOGS(s); LOGS(") => ")
#define LOG_DIC(i)      LOGS("dict["); LOG(i); LOGS("] ");  \
                        LOGS(dict[i].name); LOGS(" attr="); \
                        LOGX(dict[i].attr); LOGS("\n")
///@}
#endif // __EFORTH_SRC_CONFIG_H
