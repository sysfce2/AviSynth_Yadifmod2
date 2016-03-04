#yadifmod2
## Yet Another Deinterlacing Filter mod  for Avisynth2.6/Avisynth+

	yadifmod2 = yadif + yadifmod

###Info:

	version 0.0.0

###Requirement:
	- Avisynth2.6.0final/Avisynth+r1576 or greater.
	- WindowsVista SP2 or later.
	- Visual C++ Redistributable Packages for Visual Studio 2015.

###Syntax:

	yadifmod2(clip, int "order", int "field", int "mode", clip "edeint", int "opt")

####	clip -

		All planar formats( YV24 / YV16 / YV12 / YV411 / Y8 ) are supported.

####	order -

		Set the field order.

		-1(default) = use avisynth's internal parity value
		 0 = bff
		 1 = tff

####	field -

		Controls which field to keep when using same rate output.

		-1(default) = set eqal to order
		 0 = keep bottom field
		 1 = keep top field

		This parameter doesn't do anything when using double rate output.

####	mode -

		Controls double rate vs same rate output, and whether or not the spatial interlacing check is performed.

		0(default) = same rate, do spatial check
		1 = double rate, do spatial check
		2 = same rate, no spatial check
		3 = double rate, no spatial check

####	edeint -

		Clip from which to take spatial predictions.

		If this is not set, yadifmod2 will generate spatial predictions itself as same as yadif.

		This clip must be the same width, height, and colorspace as the input clip.
		If using same rate output, this clip should have the same number of frames as the input.
		If using double rate output, this clip should have twice as many frames as the input.

####	opt -

		Controls which cpu optimizations are used.

		-1 = autodetect
		 0 = force C routine
		 1 = force SSE2 routine
		 2(default) = force SSE2 + SSSE3 routine
		 3 = force AVX2 routine

		Since Avisynth2.6.0 has a memory alignment bug, it is almost impossible to use AVX2 on Avisynth2.6.0.
		Avisynth+ is fixed this already.
		Thus, don't set this to -1 or 3 if you are using Avisynth2.6.0.

###Note:

		- yadifmod2_avx.dll is for AVX supported CPUs.(it is compiled with /arch:AVX).

###Source code:

	https://github.com/chikuzen/yadifmod2/

