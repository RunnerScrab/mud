#ifndef UTILS_H_
#define UTILS_H_

#if !defined max && !defined __cplusplus
#define max(a,b) ((a>b)?a:b)
#endif

#if !defined min && !defined __cplusplus
#define min(a,b) ((a<b)?a:b)
#endif

#ifdef DEBUG
#define dbgprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbgprintf(...) do{}while(0)
#endif

#define RETURNFAIL_IF(a) if(a){return -1;}

#endif
