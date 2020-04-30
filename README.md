# LibECBUFR: Environment Canada BUFR Library

It is designed as a simple to use, Templates oriented, general purposes
library for reading and writing BUFR messages.  Using one single interface
for both reading and writing.

## History

libECBUFR originated from an older BUFR encoding library which was created
in 1994 within a project named GEWEX aka MOLTS (Model Output Local Time
Series) in which a BUFR encoder was needed to convert model forecast time
series into BUFR files for exchange purposes.  At that time, the library
was only capable of encoding BUFR.  At the beginning of summer 2007,
it became apparent that a number of needs across Environment Canada
could only be met with a unified approach to BUFR programming.
Soon afterwards, the need for a library capable of reading and writing BUFR
files based on templates was identified. A national requirement analysis
and a survey of available free and commercial software was conducted.
The conclusion was that while there are excellent BUFR software packages
available, the needs expressed in the Canadian context would be best met
with in-house software.

At the end of summer 2007, a prototype of the library was created
using the old BUFR encoding library. Also, new decoding functions along
with template support functions were added to the library.  At the end
of october (after 2 months), a working new BUFR library was born, which
allows applications to easily read and writes BUFR files using templates.

See the Examples directory for how to encode and decode BUFR using
this library.

Get the most recent copy of libECBUFR at https://github.com/ECCC-MSC/libecbufr

## INSTALLATION

### Requirements

The following tools (packages) are required in order to compile this library:
   
- autotools
- doxygen
- libtool
- gettext (optional, for internationalisation, included in modern glibc's) 
- autopoint (needed on newer Debian distributions)

Before anything else, you should type `./reconf`.  This will generate all needed
files to configure and install the package.

For more details, see `INSTALL`


## Note to Developers who Wish to Contribute

### Software Used

- The autotools (autoconf, automake, configure) are used to generate 
  the Makefiles.
  http://www.gnu.org/software/autoconf/manual/
  http://www.gnu.org/software/automake/manual/

- Libtool is used with the autotools to generate the libraries 
  automatically for any platform.
  http://www.gnu.org/software/libtool/manual/

- Doxygen is used to automatically generate documentation from source code 
  comments.
  http://www.doxygen.org/

### Notes on internationalisation (i18n)

LibECBUFR is internationalised with "gettext". 

Currently supported languages:

* English
* French (fr_CA)

To use the library in the language of your choice, the language must be supported
and you must set the `LC_ALL` environment variable to the appropriate value.
Ex: `export LC_ALL=fr_CA`.  That's all.

To use the internationalisation of the library from your own application, 
you must put this include in your C source file:

```c
#include "bufr_i18n.h"
```

you must also type the following lines to initialise the internationalisation:

```c
// setup for internationalization
bufr_begin_api();
setlocale (LC_ALL, "");
```

With these steps done, your own program will not be internationalised, but the
messages coming from the library will be displayed in the chosen language.
You can then internationalise your own program if you wish.  You can look at
the bufr_decoder.c and bufr_encoder.c files to see an example of how this is 
done.

#### For developpers
When you want a string to be internationalised in libECBUFR, you need to replace the usual
string by the gettext format, otherwise it will only appear in english.
Example: "This is a string" becomes _("This is a string").
Also, plural forms depending on a variable are defined like this:

`_n("Here is %d file", "Here are %d files", nb_files)`k

This way, the strings become available for translation.


#### For translators
If you want to translate this library into a new language, you can contact 
the libecbufr team for us to create the basic file to be translated for your 
language, or you can create it yourself.  All translation related files are 
found in the `po` directories.  There are three of them : `po`, `Test/po`
and `Utilities/po`.  The main translations are in `po`, the translations 
for the encoder and decoder are in `Utilities/po` and the translations for
the regression tests are found in `Test/po`.

To generate a file containing the translations for a new language, go in the
appropriate `po` directory and run `msginit -l <ll_LL>` where `<ll_LL>` is 
the language code, `fr_CA` for Canadian French, for example. This will generate
a file called `fr_CA.po` with the English strings to be translated into French.
You can then edit the file and add your translations. There are several software
that helps with the editing of such a file.  The libecbufr team uses the
xemacs po-mode to help keep the file's syntax correct.

In order to make translations, you need to understand a few key concepts:
- When you see "\n" in a string, it is the "new line" character from the
C language. If it is present in the original string, it usually must be there
in the translated string (normally it is found at the end).
- Sometimes you see "%s" or "%d", these also must appear in the translated
string.  "%s" replaces a string that will be generated at run time, and "%d"
replaces an integer that will be replaced at run time.  
- Sometimes there are two english strings at once, one has the singular 
form and the other is plural. You must provide all the plural forms for 
your language based on the value of the "%d" integer provided with the string.
- If you ever need to change the order of the "%d" or "%s" symbols, there is a
special syntax for this: "Original string %s, %d", "Translated string with
inversion of variables %2$d, %1$s"
- Every time the strings in the library are modified, the translated file will
need to be updated with "msgmerge --update fr_CA.po *.pot" and all the strings 
that were modified will be marked as "fuzzy" in the translated file.  This means
that until a translator verifies that the translations are correct, those
fuzzy translations will not be used by the program and the english string will be
used instead.

To provide translations, or translation bugs, please file an [issue](https://github.com/ECCC-MSC/libecbufr/issues)
on GitHub.

For more details see: http://www.gnu.org/software/gettext/manual/gettext.html

### Unit tests, regression tests and code coverage

Unit and regression tests are performed with the "check" tool.  All you need 
to do to execute the tests is type "make check".  If you need to debug 
unit tests, set this environment variable:

```bash
CK_FORK="no"  
```

This will prevent forks and let you use "break" in the gdb debugger.

If you want to measure the tests' code coverage, you need to type:
./configure --enable-code-coverage; make; make install
This will add the "-fprofile-arcs" and "-ftest-coverage" options to gcc, providing 
special binaries that will gather statistical data on the subsequent executions.
Then you need to type "make check" to run the test suite. This will generate
*.gcda files containing the statistical data gather during the executions.
To see the results, you can type:
"gcov file.c" with file.c being the name of the source file you want to study.
gcov will create a "file.gcov" text file that is similar to the C source code
but with added information in the left column.
For this to work, you need to compile your executable statically against the libECBUFR
library.  Dynamic linking won't work for code coverage measurement.

## Contributing

This project follows certain standards.  Here is a list of standards that
should be followed when contributing to the libECBUFR project:

- The `NEWS` file should contain a dated annoucement for every public release.
- When a bug or an issue is found, always report it
- Any public release of libECBUFR should have a different version.
- The major version number (first number) should be changed when libECBUFR 
  acquires such a major change that it loses some backward compatibility. 
- The minor version number (second number) should be changed when 
  the library acquires a new fonctionality, but maintains backward 
  compatibility. Something new from the user's point of view, like a new
  argument added to a function is enough to change the minor version number.
- The micro version number (third number) should be changed when bugs are 
  fixed. Essentially, when the micro version number changes, one should 
  assume that the library does exactly the same thing but has clearer 
  documentation, fewer bugs, something might be implemented differently, 
  but it should be transparent for the user, etc...
- When one is about to commit a change, one should make sure 
  the change is recorded in the Changelog file, and the entry in bug tracking 
  is updated before proceeding.
- Don't forget to update your working directory before commiting any changes,
  if possible.
- When you work in a branch for a modification that isn't trivial, 
  you should merge the bugfixes from the trunk regularily if relevant,
  so that you don't get yourself in a position where you need to merge 
  a huge amount of differences later on. The inverse is also true, 
  so don't forget to merge back in the trunk the bugs you fix in your branch.
- Contributions from parties outside Environment Canada may be considered for
  merge via GitHub pull requests.  If the contributed code source is distinct
  from the bulk of already-existing libECBUFR code, contributors may choose to
  retain their copyright, as long as they contribute the code under the LGPL,
  Version 3. If the code is mixed with existing Environment Canada code, we would
  kindly ask that you consider assigning the copyright to Environment Canada.
