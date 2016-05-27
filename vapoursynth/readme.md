#yadifmod2
## Yet Another Deinterlacing Filter mod  for VapourSynth

	yadifmod2 = yadif + yadifmod

### Info:

	version 0.0.0

### Requirement:
	- VapourSynth r30 or later.

### Syntax:

	ym2.yadifmod2(clip clip[, int order, int field, int mode, clip edeint, int opt])

####	clip -

		Constant format only.
		All formats except half precision are supported.

####	order -

		Set the field order.

		 0 = bff
		 1 = tff(default)

####	field -

		Controls which field to keep when using same rate output.

		-1 = set eqal to order(default)
		 0 = keep bottom field
		 1 = keep top field

		This parameter doesn't do anything when using double rate output.

####	mode -

		Controls double rate vs same rate output, and whether or not the spatial interlacing check is performed.

		0 = same rate, do spatial check(default)
		1 = double rate, do spatial check
		2 = same rate, no spatial check
		3 = double rate, no spatial check

####	edeint -

		Clip from which to take spatial predictions.

		If this is not set, yadifmod2 will generate spatial predictions itself as same as yadif.

		This clip must be the same width, height, and format as the input clip.
		If using same rate output, this clip should have the same number of frames as the input.
		If using double rate output, this clip should have twice as many frames as the input.

####	opt -

		Controls which cpu optimizations are used.

		 0 = Use C++ routine.
		 1 = Use SSE2 + SSE routine if possible. When SSE2 can't be used, fallback to 0.
		 2 = Use SSSE3 + SSE2 + SSE routine if possible. When SSSE3 can't be used, fallback to 1.
		 3 = Use SSE4.1 + SSSE3 + SSE2 + SSE routine if possible. When SSE4.1 can't be used, fallback to 2.
		 4 = Use SSE4.1 + SSSE3 + SSE2 + AVX routine if possible. When AVX can't be used, fallback to 3.
		 others = Use AVX2 + AVX routine if possible. When AVX2 can't be used, fallback to 4.(default)

### Changelog:

	0.0.0(20160515)
		initial release


###Source code:

	https://github.com/chikuzen/yadifmod2/

