# make it possible to do a verbose build by running `make V=1`
ifeq ($(V),1)
Q=
MSG=@true
else
Q=@
MSG=@$(ECHO)
endif

