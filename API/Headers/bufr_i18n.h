/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2010.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2010.

This file is part of libECBUFR.

    libECBUFR is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License,
    version 3, as published by the Free Software Foundation.

    libECBUFR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public
    License along with libECBUFR.  If not, see <http://www.gnu.org/licenses/>.
 
 *  file      :  BUFR_I18N.H
 *
 *  author    :  Michel Van Eeckhout
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADER FILE FOR BUFR INTERNATIONALIZATION
 *
 *
 */

#ifndef _bufr_i18n_h_
#define _bufr_i18n_h_

#include "config.h"
#include "gettext.h"

#define _(String) dgettext(PACKAGE, String)
#define _n(String1, String2, n) dngettext(PACKAGE, String1, String2, n)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#endif
