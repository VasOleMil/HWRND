#include <bitset>       //bit manipulation and output
#include <conio.h>
#include <cstdlib>      //std::atoi, input check and conversion
#include <fstream>      //std::ofstream, file processing
#include <iomanip>      //std::fixed and std::setprecision, adjustment 
#include <iostream>
#include <intrin.h>     // Header for CPUID  intrinsics
#include <immintrin.h>  // Header for RDRAND intrinsics
//--------------------------------------------------------------------
char            Fn[] = "RDRAND.JSON";
std::ofstream   Fs;     //file stream
int             Pr = 15;//output precision
//--------------------------------------------------------------------
bool 
TRDRAND()
{
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 1); // CPUID with EAX = 1
    return (cpuInfo[2] & (1 << 30)) == 0; // Check ECX bit 30
}//RDRAND support check
//--------------------------------------------------------------------
int  
main(int argc, char* argv[])
{
    // Default number of steps
    int s = 100; unsigned long long t;

    // Check if argument is provided
    if (argc > 1) 
    {
        try 
        {
            s = std::atoi(argv[1]); // Convert argument to integer
            if (s <= 0) throw std::invalid_argument("Invalid step count");
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Error: Invalid number of steps. Using default: " << s << std::endl;
        }
    }
    else
    {
        std::cout << "Default steps, changes using: HWRNG.exe [Steps]" << std::endl;
    }   
    
    // Test RDRAND support and generate test number
    if (TRDRAND())
    {
        std::cout << "RDRAND is NOT supported on this processor." << std::endl; return 1;
    }
    else if (_rdrand64_step(&t)) //<immintrin.h>
    {
        std::cout << "RDRAND test: \n\t\t" << t << std::endl;
    }
    else 
    {
        std::cout << "RDRAND failed to generate a random number." << std::endl; return 1;
    }
    //Open file and write array header
    try 
    {
        Fs.open(Fn);

        if (!Fs.is_open()) 
        {
            throw std::ios_base::failure("Error opening file.");
        }

        // Write array header to file, JSON structure "{\"values\":[]}"
        Fs << "{\"values\":[";

        std::cout << "File container: \n\t\t" << Fn << std::endl;
    }
    catch (const std::ios_base::failure& e) 
    {
        std::cerr << "File operation error: " << e.what() << std::endl;
    }
    catch (...) 
    {
        std::cerr << "An unknown error occurred while handling the file." << std::endl;
    }
                        
    unsigned long long eh = 0x4320000000000000ULL;  // exp for 2^53-1
    unsigned long long ms = 0x8000000000000000ULL;  // sign mask hex
    unsigned long long mh = 0xFFFFFFFFFFFFFULL;     // mantissa mask
    unsigned long long nh = 0x432FFFFFFFFFFFFFULL;  // 2^53-1 in double
    unsigned long long hh = 0x3FE0000000000000ULL;  // 0.5 shift in hex

    double d, n, h, m, a, z, Q, P, sz, sd, zz; unsigned long long r, * dp, * sp;
    
    //Write n, h values and link dp, sp pointers
    dp = (unsigned long long*) & n; *dp = nh; //write denominator
    dp = (unsigned long long*) & h; *dp = hh; //write shift
    sp = (unsigned long long*) & m; *sp =(ms & t);      //link sign, save t sign
    dp = (unsigned long long*) & d; *dp = eh | (mh & t);//link rest, save t rest
    
    //Process generaeted by test value, init saved values
    { d /= n; d -= h; *dp = *sp | *dp; } { zz = d * d; z = d; }
    //write processed value to file
    Fs << std::fixed << std::setprecision(Pr) << d; if (s > 1) Fs << ",";

    t = 0x1FFFFFFFFFFFFF; //Zero Mask and Value case, bit 53
    long i, c = 0; Q = 0.0F; a = 0.0F; sd = sz = 0.0F;
    
    
    for (i = 2; i <= s; i++)
    {
        if (_rdrand64_step(&r)) //<immintrin.h>
        {
            *sp = (ms & r);     //save sign in m
            *dp = eh | (mh & r);//save rest in d
            //normalise, shift, sign
            d /= n; d -= h; *dp =  *sp | *dp;
            //Zero case, probability 2^(-53)
            d *= (t != (r & t));

            //Usign rest [63-53] exponent bits for tail
            //t = (r & 0x7FE0000000000000ULL) >> 53;
            //*dp = t | (*dp & 0xFFFFFFFFFFFFFC00ULL);
            
            // Copilot version, tested on bias:
            // Normalize to [0,1] using all 52 bits
            // d = (r & 0xFFFFFFFFFFFFF) / (double)(1ULL << 52);
            // Shift to [-0.5, +0.5]
            // d -= 0.5;
            // Apply sign bit randomly (optional)
            // if (r & (1ULL << 63)) d = -d;

            // Write value to JSON with precision
            Fs << std::fixed << std::setprecision(Pr) << d;
            if (i < s) Fs << ","; // Separate with commas

            //Analitic block: average, bias, Pearson 
            a += d; c++; Q += d * z;  
            sz += zz; 
            sd += zz = d * d; z = d;          
        }       
    }

    Fs << "]}"; Fs.close(); //Close JSON structure and file
    
    // Certificate prepare
    // Thus at c = 0, test value processed: 
    // ((c-1)/c) -> c/(c+1) for sz
    // ((c-1)/c) for sd, previus selection is unit greater
    sd = (sd == 0.0F) ? 1.0F : sd;      //zero safe
    sz = (sz == 0.0F) ? 1.0F : sz;      //zero safe
    a /= (c > 0) ? ((double)c) : 1.0F;  //zero safe

    P = (c > 1) ? (Q / sqrt((c - 1) / (c + 1) * sd * sz)) : 0.0F;
    
    // Certificate supply
    std::cout << "Steps:\t\t" << s << std::endl;
    std::cout << "Values:\t\t" << c + 1 << std::endl;
    std::cout << "fails:\t\t" << s - c - 1 << std::endl;
    std::cout << "E:\t\t" << a << std::endl;
    std::cout << "Q:\t\t" << Q << std::endl;
    std::cout << "P:\t\t" << P << std::endl;

    //Test edge covering by division
    //*dp = nh; c = (d / n - 1.0F == 0.0F);
    //std::cout << "(d/n)-1 == 0 :\t" << c << std::endl;
    
    //Developer handies, hex visual examinantion
    
    //d = 0xFFFFFFFFFFFFF; //unit mantissa expeted
    //std::cout << "db\t" << std::bitset<64>(*dp) << std::endl;
    //std::cout << "dd\t" << (d) << std::endl;
     
    //std::cout << "mh\t" << std::bitset<64>(mh) << std::endl;
    //std::cout << "eh\t" << std::bitset<64>(eh) << std::endl;
    //std::cout << "nh\t" << std::bitset<64>(nh) << std::endl;
    
    std::cout << "Press sny key to continue..." << _getch();

    return 0;
}
//--------------------------------------------------------------------
//bool 
//GetRandomNumber(unsigned long long* randomNumber)
//{
//    return _rdrand64_step(randomNumber); 
//}// Call RDRAND for a 64-bit random number
//--------------------------------------------------------------------
//int 
//GetMantisse(void)
//{
//    double  Unit = 1.00F;
//    double  Step = 2.00F;
//    double  Shift = Unit;
//
//    int Mantisse = 1;
//
//    while (Unit + Shift != Unit)
//    {
//        Shift /= Step; Mantisse++;
//    }
//    //exclude leading and round bits
//    return Mantisse - 2;
//}//Get IEEE 754 effective float point mantisse
//--------------------------------------------------------------------