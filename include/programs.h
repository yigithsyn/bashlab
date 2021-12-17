#ifndef H_PROGRAMS
#define H_PROGRAMS

static const char *programs =
    "["
    "{"
    "\"name\": \"s11toswr\","
    "\"desc\": \"Convert microwave/rf S11 parameters to standing wave ratio (SWR)\","
    "\"vers\": ["
    /*        */"{\"num\":\"1.0.0\", \"msg\":\"Initial release\"}"
    /*      */"],"
    "\"pargs\": ["
    /*        */"{\"name\":\"s11\", \"minc\":1, \"maxc\":100, \"desc\":\"Reflection coefficient (S11) value(s)\"}"
    /*       */"],"
    "\"oargs\": ["
    /*        */"{\"short\":\"d\", \"long\":\"db\", \"minc\":0, \"maxc\":1, \"desc\":\"Parse S11 value(s) in decibel [dB]\"}"
    /*       */"]"
    "}"
    "]";

#endif