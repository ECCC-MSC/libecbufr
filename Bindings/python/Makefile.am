
all-local:
	(cd $(srcdir); $(PYTHON) setup.py build_ext -i \
     --verbose)

install-exec-local:
	( $(PYTHON) setup.py install \
   --prefix $(DESTDIR)$(prefix) --verbose )

clean-local:
	(rm -f ecbufr/*.so ecbufr/*.c  ecbufr/*.pyc)




