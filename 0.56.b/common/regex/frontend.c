#include "regexp_int.h"
#include "regexp_custom.h"

/* frontend for re_ functions (narrow char versions) */

#ifdef REGEXP_UNICODE

#define	FAIL(m, code)		{ re_report(m); return code; }
#define FAIL2(m, code)      { re_report(m); if(errp) *errp = code; }

int re_comp(regexp** rpp, const char* regex)
{
    CHAR_TYPE* regex_w = NULL;
    int retval;

    if(regex)
    {
        regex_w = re_ansi_to_unicode(regex);
        if(!regex_w)
            FAIL("Unable to convert regex to wide characters", REGEXP_EEND);
    }
    
    retval = re_comp_w(rpp, regex_w);
    re_cfree(regex_w);
    return retval;
}

int re_exec(const regexp* rp, const char* s, size_t nmatch, regmatch pmatch[])
{
    CHAR_TYPE* s_w = NULL;
    int retval;

    if(s)
    {
        s_w = re_ansi_to_unicode(s);
        if(!s_w)
            FAIL("Unable to convert match string to wide characters", REGEXP_EEND);
    }

    retval = re_exec_w(rp, s_w, nmatch, pmatch);
    re_cfree(s_w);
    return retval;
}

int re_sub(const regexp* rp, const char* s, const char* src, char** sub)
{
    CHAR_TYPE* s_w = NULL;
    CHAR_TYPE* src_w = NULL;
    CHAR_TYPE* sub_w = NULL;
    
    int retval;

    if(s)
    {
        s_w = re_ansi_to_unicode(s);
        if(!s_w)
            FAIL("Unable to convert match string to wide characters", REGEXP_EEND);
    }
    if(src)
    {
        src_w = re_ansi_to_unicode(src);
        if(!src_w)
            FAIL("Unable to convert substitute string to wide characters", REGEXP_EEND);
    }
    if(!sub)
        retval = re_sub_w(rp, s_w, src_w, NULL);
    else
    {
        *sub = NULL;
        retval = re_sub_w(rp, s_w, src_w, &sub_w);
    }
    if(retval >= 0 && sub_w)
    {
        *sub = re_unicode_to_ansi(sub_w);
        re_cfree(sub_w);
    }
    
    re_cfree(src_w);
    re_cfree(s_w);

    return retval;
}

#endif
