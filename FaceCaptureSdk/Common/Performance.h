
#ifndef _PERFORMANCE_HEADER_H_
#define _PERFORMANCE_HEADER_H_

#if defined(DEVELOPMENT)

#define PRINT printf
#define DO_IF(exp, cond) if (cond) {\
        exp;\
    }

#else

#define PRINT
#define DO_IF(exp, cond)

#endif

#ifdef LOG_PERFORMANCE

#define START_EVALUATE(module) clock_t clock_t_##module = clock()
#define PRINT_COSTS(module) PRINT("%s::%s costs: %d\n", __FUNCTION__, #module, clock() - clock_t_##module)
#define PRINT_TAG_COSTS(module, tag) PRINT("%s::%s::%s costs: %d\n", __FUNCTION__, #module, #tag, clock() - clock_t_##module)
#define COSTS(module) (clock() - clock_t_##module)

#define START_FUNCTION_EVALUATE() clock_t clock_t_this_function = clock()
#define PRINT_FUNCTION_COSTS() PRINT("%s costs: %d\n", __FUNCTION__, clock() - clock_t_this_function)
#define PRINT_FUNCTION_TAG_COSTS(tag) PRINT("%s::%s costs: %d\n", __FUNCTION__, #tag, clock() - clock_t_this_function)

#else

#define START_EVALUATE(module) 
#define PRINT_COSTS(module) 
#define PRINT_TAG_COSTS(module, tag) 
#define COSTS(module) 

#define START_FUNCTION_EVALUATE() 
#define PRINT_FUNCTION_COSTS() 
#define PRINT_FUNCTION_TAG_COSTS(tag) 

#endif

#ifdef TRACK_PERFORMANCE
#define TRACK PRINT
#else
#define TRACK 
#endif

#endif

