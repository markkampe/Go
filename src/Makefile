OBJS=main.o board.o move.o admin.o mechanix.o influenc.o libs.o\
	pcdos.o versn.o dbg.o help.o d_pcdos.o

go:  $(OBJS)
	gcc -o $@ $(OBJS)

clean:
	rm -f $(OBJS)

admin.o: go.h

board.o: go.h disp.h

dbg.o: dbg.h go.h libs.h strings.h move.h

influenc.o: go.h inf.h disp.h

libs.o: go.h libs.h move.h

main.o: go.h 

move.o: go.h move.h disp.h

mechanix.o: go.h move.h disp.h strings.h dbg.h

d_pcdos.o: go.h disp.h
