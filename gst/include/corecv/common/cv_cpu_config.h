// OpenCV CPU baseline features
#ifndef __CV_CPU_CONFIG_H__
#define __CV_CPU_CONFIG_H__

#ifdef USE_NEON
#define CV_CPU_COMPILE_NEON 1
#define CV_CPU_BASELINE_COMPILE_NEON 1
    #ifdef USE_FP16
    	#define CV_CPU_COMPILE_FP16 1
		#define CV_CPU_BASELINE_COMPILE_FP16 1
		#define CV_CPU_BASELINE_FEATURES 0 \
    		, CV_CPU_NEON \
    		, CV_CPU_FP16 
    #else
    	#define CV_CPU_BASELINE_FEATURES 0 \
    		, CV_CPU_NEON 
    #endif
#else
	#define CV_CPU_BASELINE_FEATURES
#endif

#ifdef USE_IVE
    #define CV_USE_IVE
#endif    

#endif // __CV_CPU_CONFIG_H__
