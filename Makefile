CC = gcc -O3
BUILDDIR = build
BINDIR = bin
MODDIR = modules
TESTDIR = public
MODULES = GET.c POST.c
TESTS = helloworld.c reflect.c
FRAMEWORK = dispatch.c server.c process_request.c request.c helpers.c
MODULE_OBJS = $(MODULES:%.c=$(BUILDDIR)/$(MODDIR)/%.o)
FRAMEWORK_OBJS = $(FRAMEWORK:%.c=$(BUILDDIR)/%.o)
TESTPROGS = $(TESTS:%.c=$(TESTDIR)/%)
EXEC = server

all: directories ${TESTPROGS} ${EXEC}

directories:
	mkdir -p ${BUILDDIR}/${MODDIR} ${BINDIR}

public/%: public/%.c
	${CC} $< -o $@

${EXEC}: ${FRAMEWORK_OBJS} ${MODULE_OBJS}
	${CC} ${FRAMEWORK_OBJS} ${MODULE_OBJS} -o ${BINDIR}/${EXEC}

${BUILDDIR}/server.o: ${FRAMEWORK} ${BUILDDIR}/dispatch.o \
	                    ${BUILDDIR}/process_request.o \
	                    ${BUILDDIR}/request.o \
	                    ${BUILDDIR}/helpers.o
	${CC} -c server.c -o ${BUILDDIR}/server.o

${BUILDDIR}/dispatch.o: dispatch.c dispatch.h ${MODULE_OBJS} 
	${CC} -c dispatch.c -o ${BUILDDIR}/dispatch.o

${BUILDDIR}/process_request.o: process_request.c process_request.h
	${CC} -c process_request.c -o ${BUILDDIR}/process_request.o

${BUILDDIR}/request.o: request.c request.h 
	${CC} -c request.c -o ${BUILDDIR}/request.o

${BUILDDIR}/helpers.o: helpers.c helpers.h
	${CC} -c helpers.c -o ${BUILDDIR}/helpers.o

${BUILDDIR}/${MODDIR}/%.o: ${MODDIR}/%.c ${MODDIR}/%.h
	${CC} -c $< -o $@

run:
	bin/server -v public


