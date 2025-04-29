# HWRND
RD-RAND provider for IEEE 754 double.

Shell command line tool to generate sequence of floats in range [-0.5:+0.5]. Calling generates RDRAND.JSON in executable folder.

Usage: 
           
           HWRNG.exe [Steps]
           
           HWRNG.exe [Steps] [Any_Value], to suppress key waiting

Details:
           
           If no parameters are provided, the JSON file contains 100 doubles (approximately 2KB in size).

           Program returns immidiately if RDRAND is not detected, use HWRNG.cmd to view console messages.


Implementation uses hardware RDRAND for setting 52 bit mantissa and sign of double. Normalisation is implemented by division. Not all source bits used. 
Preliminary distribution quality can be controlled in console report: Average, Correlation, Pearson coefficient.

Tool can be used for checking device RDRAND capability, access to hardware implemented by _rdrand64_step(&t) in <immintrin.h>

Technical Details: 

Peculiarity of this implementation is usage of same divisor exponent, that extends range to boundary. Using the remaining ten 'exponent' bits [53-62] of the source for the mantissa tail after normalization - did not show any influence on correlation. These bits can be stored and potentially reused in more advanced versions.

Main conversion loop:

{ d /= n; d -= h; *dp = *sp | *dp; } //normalise, shift, sign

{ d *= (t != (r & t)); } //zero case, probability 2^(-53)
           
Variable values:

dp=&d, sp=&m; //initial double values are set through integer pointers

t = 0x1FFFFFFFFFFFFF; //zero mask and value, bit 52

nh = 0x432FFFFFFFFFFFFFULL; //devisor in Hex

hh = 0x3FE0000000000000ULL; //shift in Hex

Sign is obtained as major bit of 64 bit RDRAND integer. 



Project developped in MS VS 2022 Community, C++ console, Windows x64. Executable relies on MSVC runtime libraries; ensure they are installed or redistribute them according to Microsoft's licensing terms.
