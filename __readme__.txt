--------------------------------------------------------------------------------
Part A. Windows 10에 gcc 설치하기
--------------------------------------------------------------------------------
https://www.mingw-w64.org/downloads/

1. 먼저 위 링크에 가서
		---> MingW-W64-builds 를 클릭
			...
			Mingw-builds
				Installation: [GitHub] 	//<---클릭
					---> Release of 13.2.0-rt_v11-rev1
						---> x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1.7z 를 다운&압축해제

	확인:
		C:\Users\smubp\Downloads\x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1\mingw64\bin


2. 시스템 환경변수 편집
	환경변수 설정창을 Open
		--->[시스템 변수(S)] 의 [Path] 변수를 선택
			---> [새로 만들기] 를 눌러서, 위의 Path 를 등록한다.


3. GNU GCC가 설치되었는지 확인
	C:\Users\smubp>gcc -v
	Using built-in specs.
	COLLECT_GCC=gcc
	COLLECT_LTO_WRAPPER=C:/Users/smubp/Downloads/x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1/mingw64/bin/../libexec/gcc/x86_64-w64-mingw32/13.2.0/lto-wrapper.exe
	Target: x86_64-w64-mingw32
	Configured with: ../../../src/gcc-13.2.0/configure --host=x86_64-w64-mingw32 --build=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 --prefix=/mingw64 --with-sysroot=/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64 --enable-host-shared --disable-multilib --enable-languages=c,c++,fortran,lto --enable-libstdcxx-time=yes --enable-threads=win32 --enable-libstdcxx-threads=yes --enable-libgomp --enable-libatomic --enable-lto --enable-graphite --enable-checking=release --enable-fully-dynamic-string --enable-version-specific-runtime-libs --enable-libstdcxx-filesystem-ts=yes --disable-libssp --disable-libstdcxx-pch --disable-libstdcxx-debug --enable-bootstrap --disable-rpath --disable-win32-registry --disable-nls --disable-werror --disable-symvers --with-gnu-as --with-gnu-ld --with-arch=nocona --with-tune=core2 --with-libiconv --with-system-zlib --with-gmp=/c/buildroot/prerequisites/x86_64-w64-mingw32-static --with-mpfr=/c/buildroot/prerequisites/x86_64-w64-mingw32-static --with-mpc=/c/buildroot/prerequisites/x86_64-w64-mingw32-static --with-isl=/c/buildroot/prerequisites/x86_64-w64-mingw32-static --with-pkgversion='x86_64-win32-seh-rev1, Built by MinGW-Builds project' --with-bugurl=https://github.com/niXman/mingw-builds CFLAGS='-O2 -pipe -fno-ident -I/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/include -I/c/buildroot/prerequisites/x86_64-zlib-static/include -I/c/buildroot/prerequisites/x86_64-w64-mingw32-static/include' CXXFLAGS='-O2 -pipe -fno-ident -I/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/include -I/c/buildroot/prerequisites/x86_64-zlib-static/include -I/c/buildroot/prerequisites/x86_64-w64-mingw32-static/include' CPPFLAGS=' -I/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/include -I/c/buildroot/prerequisites/x86_64-zlib-static/include -I/c/buildroot/prerequisites/x86_64-w64-mingw32-static/include' LDFLAGS='-pipe -fno-ident -L/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/lib -L/c/buildroot/prerequisites/x86_64-zlib-static/lib -L/c/buildroot/prerequisites/x86_64-w64-mingw32-static/lib ' LD_FOR_TARGET=/c/buildroot/x86_64-1320-win32-seh-ucrt-rt_v11-rev1/mingw64/bin/ld.exe --with-boot-ldflags=' -Wl,--disable-dynamicbase -static-libstdc++ -static-libgcc'
	Thread model: win32
	Supported LTO compression algorithms: zlib
	gcc version 13.2.0 (x86_64-win32-seh-rev1, Built by MinGW-Builds project)









--------------------------------------------------------------------------------
Part B. Windows 10에 make 설치하기
--------------------------------------------------------------------------------
http://gnuwin32.sourceforge.net/packages/make.htm

1. 먼저 위 링크에 가서,
	Complete package, except sources의 [Setup]을 클릭하여, GNU Make를 설치한다

	단, 설치하는 장소를 기억해야한다.

	My Path:
		C:\Program Files (x86)\GnuWin32\bin


2. 시스템 환경변수 편집
	환경변수 설정창을 Open
		--->[시스템 변수(S)] 의 [Path] 변수를 선택
			---> [새로 만들기] 를 눌러서, 위의 Path 를 등록한다.


3. GNU Make가 설치되었는지 확인
	cmd.exe 를 열고, make -v 를 입력해서 다음 Message 가 나오면 OK:

		C:\Users\smubp>make -v
		GNU Make 3.81
		Copyright (C) 2006  Free Software Foundation, Inc.
		This is free software; see the source for copying conditions.
		There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
		PARTICULAR PURPOSE.

		This program built for i386-pc-mingw32







--------------------------------------------------------------------------------
Part C. Windows 10에 pthread 설치하기
--------------------------------------------------------------------------------
Q: windows 10 에서 pthread 를 별도 등록없이 사용이 가능한가?
A:
	Windows 환경에서 POSIX 스레드(POSIX Threads, pthreads)는 기본적으로 지원되지 않으며, 별도의 라이브러리를 설치해야 한다.
	pthread는 주로 Unix 계열 시스템에서 사용되기 때문에 Windows에서 직접적으로 사용할 수는 없다.
	하지만 Windows에서 POSIX 스레드를 사용하는 방법은 있다.


----------------------------------------
Windows에서 POSIX 스레드를 사용하는 방법:
----------------------------------------
www.sourceware.org/pthreads-win32/

1. 먼저 위 링크에 가서
	Where can I get it?
	Download
	[You can also get it from one of several mirror sites.]		//<--- mirror 클릭

	제 경우에는 다운로드가 되지 않아 Germany mirror 사이트로 들어가서 다운받았다.

	Germany mirror 사이트:
		https://ftp.gwdg.de/pub/linux/sources.redhat.com/
			---> pthreads-win32/
				---> pthreads-w32-2-9-1-release.zip	//<--- 이 파일 다운로드

2. pthreads-w32-2-9-1-release.zip 를 압축해제하고, 아래 2 폴더를 본인의 프로젝트 Directory에 복사한다.
	lib 폴더:
		pthreads-w32-2-9-1-release/Pre-built.2/lib
	include 폴더:
		pthreads-w32-2-9-1-release/Pre-built.2/include


--------------------
컴파일 및 링크:
--------------------
	gcc에
	-I option를 사용하여 include Directory를 지정하고,
	-L option를 사용하여 lib Directory를 지정한다.

--------------------
예시 Makefile
--------------------
CC = gcc
CFLAGS = -I/path/to/pthreads-win32/include -Wall
LDFLAGS = -L/path/to/pthreads-win32/lib -lpthread

hello: hello.o
	$(CC) hello.o -o hello $(LDFLAGS)

hello.o: hello.c
	$(CC) $(CFLAGS) -c hello.c


--------------------
POSIX 스레드를 사용한 간단한 C 예제코드
--------------------
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg)
{
	printf("Hello from thread!\n");
	return NULL;
}

int main()
{
	pthread_t thread;
	pthread_create(&thread, NULL, thread_function, NULL);
	pthread_join(thread, NULL);
	printf("Hello from main!\n");
	return 0;
}

