clean-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		rm -rf clasp ; \
		rm -rf gringo ;\
	fi

all-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		TOP_SRCDIR=$(top_srcdir) \
		BOOST_ROOT=$(NESTED_BOOSTROOT) \
		CXX="$(CXX)" USING_CLANG=$(using_clang) \
		$(SHELL) $(top_srcdir)/build_potassco.sh ; \
	fi

distclean:
	-rm -rf Makefile

EXTRA_DIST = main.cc.patch SConstruct.patch SConstruct-lua.patch SConstruct-python.patch #gringo-clang.patch gringo.patch gringo-patch-cond.diff gringo-patch-domain-fwd-decl.diff gringo-patch-domain-fwd-decl-builtsource.diff gringo-patch-unpool-pred.diff

