#ifndef LANG_H
#define LANG_H

/* 
  Locale support for strings displayed on the OLED display

  Two languages supported at this point:
    English - lang_en.h
    French  - lang_fr.h

  To add a language, use one of the above files as a template
  and translate its macro definitions. Then save the file
  with an appropriate name such as
  
     lang_en.h      // two-letter ISO 639-1 language code
  
  or 

     lang_en-GB.h  //  Tags for Identifying Languages https://tools.ietf.org/rfc/bcp/bcp47.txt

  alongside the previously defined language headers. Add the
  file name to the list of language header files and choose the
  wanted language.  

*/


// **** Include one and only one NLS file **** 

#include "lang_en.h"    // English
//#include "lang_fr.h"      // French
//#include "lang_xxxx.h"  // 

#endif