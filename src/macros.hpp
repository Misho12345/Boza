#pragma once

#define STATIC_VARIABLE_FN(METHOD, CONSTRUCTOR)                             \
    decltype(METHOD()) METHOD()                                             \
    {                                                                       \
        static std::remove_reference_t<decltype(METHOD())> v CONSTRUCTOR;   \
        return v;                                                           \
    }


#define STATIC_VARIABLE_FN_ARGS(RET_TYPE, METHOD, PARAMS, CONSTRUCTOR, OTHER)   \
    RET_TYPE& METHOD PARAMS                                                     \
    {                                                                           \
        OTHER                                                                   \
        static RET_TYPE v CONSTRUCTOR;                                          \
        return v;                                                               \
    }
