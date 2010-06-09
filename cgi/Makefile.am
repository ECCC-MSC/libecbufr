# FIXME: should use configure targets
cgidir=/usr/lib/cgi-bin/libecbufr/
htmldir=/var/www/libecbufr
exdir=/var/www/libecbufr/ex
docdir=/usr/share/doc/libecbufr/cgi

cgi_SCRIPTS=decode.pl template.pl

html_DATA = index.html nav.html home.html
doc_DATA = README
ex_DATA = $(wildcard ../Test/BUFR/*.bufr)

$(html_DATA): mkindex.pl
	./mkindex.pl $(EX)
