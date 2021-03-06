// Libmorton Tests
// This is a program designed to test and benchmark the functionality offered by the libmorton library
//
// Jeroen Baert 2015

// Utility headers
#include "libmorton_test.h"

// Standard headers
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <iomanip>
#include <bitset>

using namespace std;
using namespace std::chrono;

// Configuration
size_t MAX;
unsigned int times;
size_t total;
size_t RAND_POOL_SIZE = 9000;

// Runningsums
vector<uint_fast64_t> running_sums;

vector<encode_3D_64> f_64_encode; // 64-bit encode functions
vector<encode_3D_32> f_32_encode; // 32_bit encode functions

// Make a total of all running_sum checks and print it
// This is an elaborate way to ensure no function call gets optimized away
void printRunningSums(){
	uint_fast64_t t = 0;
	cout << "Running sums check: ";
	for(int i = 0; i < running_sums.size(); i++) {
		t+= running_sums[i];
	}
	cout << t << endl;
}

template <typename morton, typename coord>
static bool check3D_DecodeFunction(string method_tested, void (*decode_function)(const morton m, coord &x, coord &y, coord &z)){
	bool everything_okay = true;
	coord x, y, z;
	// check first items
	for (morton i = 0; i < 4096; i++){
		decode_function(i, x, y, z);
		if (x != control_3D_Decode[i][0] || y != control_3D_Decode[i][1] || z != control_3D_Decode[i][2]){
			printIncorrectDecoding3D<morton,coord>(method_tested, i, x, y, z, control_3D_Decode[i][0], control_3D_Decode[i][1], control_3D_Decode[i][2]);
			everything_okay = false;
		}
	}

	//// do edge test for 32 bit
	//decode_function(0x7ffffffff, x, y, z);
	//if (sizeof(morton) = 4) { // 32 bit function
	//	if()
	//}


	if (sizeof(morton) >= 4){ // Let's do some more tests
		decode_function(0x7fffffffffffffff, x, y, z);
		if (x != 0x1fffff || y != 0x1fffff || z != 0x1fffff){
			printIncorrectDecoding3D<morton, coord>(method_tested, 0x7fffffffffffffff, x, y, z, 0x1fffff, 0x1fffff, 0x1fffff);
			everything_okay = false;
		}
	}
	return everything_okay;
}

template <typename morton, typename coord>
static bool check3D_EncodeFunction(string method_tested, morton (*encode_function)(const coord x, const coord y, const coord z)){
	bool everything_okay = true;
	morton computed_code, correct_code = 0;
	for (coord i = 0; i < 16; i++){
		for (coord j = 0; j < 16; j++){
			for (coord k = 0; k < 16; k++){
				correct_code = control_3D_Encode[k + (j * 16) + (i * 16 * 16)];
				computed_code = encode_function(i, j, k);
				if (computed_code != correct_code){
					everything_okay = false;
					cout << endl << "    Incorrect encoding of (" << i << ", " << j << ", " << k << ") in method " << method_tested.c_str() << ": " << computed_code <<
						" != " << correct_code << endl;
				}
			}
		}
	}
	return everything_okay;
}

template <typename morton, typename coord>
static double testEncode_3D_Linear_Perf(morton(*function)(coord, coord, coord), size_t times){
	Timer timer = Timer();
	morton runningsum = 0;
	for (size_t t = 0; t < times; t++){
		for (coord i = 0; i < MAX; i++){
			for (coord j = 0; j < MAX; j++){
				for (coord k = 0; k < MAX; k++){
					timer.start();
					runningsum += function(i, j, k);
					timer.stop();
				}
			}
		}
	}
	running_sums.push_back(runningsum);
	return timer.elapsed_time_milliseconds / (float) times;
}

template <typename morton, typename coord>
static double testEncode_3D_Random_Perf(morton(*function)(coord, coord, coord), size_t times){
	Timer timer = Timer();
	coord maximum = ~0;
	morton runningsum = 0;
	coord x, y, z;

	for (size_t t = 0; t < times; t++){
		// Create a pool of random numbers
		vector<coord> randnumbers;
		for (size_t i = 0; i < RAND_POOL_SIZE; i++) {
			randnumbers.push_back(rand() % maximum);
		}
		// Do the performance test
		for (size_t i = 0; i < total; i++){
			x = randnumbers[i % RAND_POOL_SIZE];
			y = randnumbers[(i + 1) % RAND_POOL_SIZE];
			z = randnumbers[(i + 2) % RAND_POOL_SIZE];
			timer.start();
			runningsum += function(x,y,z);
			timer.stop();
		}
	}
	running_sums.push_back(runningsum);
	return timer.elapsed_time_milliseconds / (float) times;
}

template <typename morton, typename coord>
static std::string testEncode_3D_Perf(morton(*function)(coord, coord, coord), size_t times) {
	stringstream os;
	os << setfill('0') << std::setw(6) << std::fixed << std::setprecision(3) << testEncode_3D_Linear_Perf<morton, coord>(function, times) << " ms " 
		<< testEncode_3D_Random_Perf<morton, coord>(function, times) << " ms";
	return os.str();
}

template <typename morton, typename coord>
static double testDecode_3D_Linear_Perf(void(*function)(const morton, coord&, coord&, coord&), size_t times){
	Timer timer = Timer();
	coord x, y, z;
	morton runningsum = 0;
	for (size_t t = 0; t < times; t++){
		for (morton i = 0; i < total; i++){
			timer.start();
			function(i,x,y,z);
			timer.stop();
			runningsum += x + y + z;
		}
	}
	running_sums.push_back(runningsum);
	return timer.elapsed_time_milliseconds / (float)times;
}

template <typename morton, typename coord>
static double testDecode_3D_Random_Perf(void(*function)(const morton, coord&, coord&, coord&), size_t times){
	Timer timer = Timer();
	coord x, y, z;
	morton maximum = ~0; // maximum for the random morton codes
	morton runningsum = 0;
	morton m;

	// Create a pool of randum numbers
	vector<morton> randnumbers;
	for (size_t i = 0; i < RAND_POOL_SIZE; i++) {
		randnumbers.push_back((rand() + rand()) % maximum);
	}
	
	// Start performance test
	for (int t = 0; t < times; t++){
		for (size_t i = 0; i < total; i++){
			m = randnumbers[i % RAND_POOL_SIZE];
			timer.start();
			function(m,x,y,z);
			timer.stop();
			runningsum += x + y + z;
		}
	}
	running_sums.push_back(runningsum);
	return timer.elapsed_time_milliseconds / (float)times;
}

template <typename morton, typename coord>
static std::string testDecode_3D_Perf(void(*function)(const morton, coord&, coord&, coord&), size_t times) {
	stringstream os;
	os << setfill('0') << std::setw(6) << std::fixed << std::setprecision(3) << testDecode_3D_Linear_Perf<morton, coord>(function, times) << " ms " 
		<< testDecode_3D_Random_Perf<morton, coord>(function, times) << " ms";
	return os.str();
}

static void check3D_EncodeCorrectness() {
	printf("++ Checking correctness of 3D encoding methods ... ");
	bool ok = true;
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Shifted LUT256 ", &morton3D_Encode_LUT256_shifted<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Shifted LUT256 ET ", &morton3D_Encode_LUT256_shifted_ET<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D LUT256 ", &morton3D_Encode_LUT256<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D LUT256 ET ", &morton3D_Encode_LUT256_ET<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Magicbits", &morton3D_Encode_magicbits<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D For", &morton3D_Encode_for<uint_fast64_t>);
	ok &= check3D_EncodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D For ET", &morton3D_Encode_for_ET<uint_fast64_t>);

	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Shifted LUT256 ", &morton3D_Encode_LUT256_shifted<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Shifted LUT256 ET ", &morton3D_Encode_LUT256_shifted_ET<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D LUT256 ", &morton3D_Encode_LUT256<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D LUT256 ET ", &morton3D_Encode_LUT256_ET<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Magicbits", &morton3D_Encode_magicbits<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D For", &morton3D_Encode_for<uint_fast32_t>);
	ok &= check3D_EncodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D For ET", &morton3D_Encode_for_ET<uint_fast32_t>);
	if (ok) { printf(" Passed. \n"); }
	else { printf("    One or more methods failed. \n"); }
}

static void check3D_DecodeCorrectness() {
	printf("++ Checking correctness of 3D decoding methods ... ");
	bool ok = true;
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Shifted LUT256 ", &morton3D_Decode_LUT256_shifted<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Shifted LUT256 ET", &morton3D_Decode_LUT256_shifted_ET<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D LUT256 ", &morton3D_Decode_LUT256<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D LUT256 ET", &morton3D_Decode_LUT256_ET<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D Magicbits", &morton3D_Decode_magicbits<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D For", &morton3D_Decode_for<uint_fast64_t, uint_fast32_t>);
	ok &= check3D_DecodeFunction<uint_fast64_t, uint_fast32_t>("64bit 3D For ET", &morton3D_Decode_for_ET<uint_fast64_t, uint_fast32_t>);

	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Shifted LUT256 ", &morton3D_Decode_LUT256_shifted<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Shifted LUT256 ET", &morton3D_Decode_LUT256_shifted_ET<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D LUT256 ", &morton3D_Decode_LUT256<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D LUT256 ET", &morton3D_Decode_LUT256_ET<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D Magicbits", &morton3D_Decode_magicbits<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D For", &morton3D_Decode_for<uint_fast32_t, uint_fast16_t>);
	ok &= check3D_DecodeFunction<uint_fast32_t, uint_fast16_t>("32bit 3D For ET", &morton3D_Decode_for_ET<uint_fast32_t, uint_fast16_t>);
	if (ok) { printf(" Passed. \n"); }
	else { printf("    One or more methods failed. \n"); }
}

static void Encode_3D_Perf() {
	cout << "++ Encoding " << MAX << "^3 morton codes (" << total << " in total)" << endl;
	cout << "    64-bit LUT256 preshifted:    " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_LUT256_shifted<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256 preshifted ET: " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_LUT256_shifted_ET<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256:               " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_LUT256<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256 ET:            " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_LUT256_ET<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit Magicbits:            " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_magicbits<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit For:                  " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_for<uint_fast64_t>, times) << endl;
	cout << "    64-bit For ET:               " << testEncode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Encode_for_ET<uint_fast64_t>, times) << endl;
	cout << "" << endl;
	cout << "    32-bit LUT256 preshifted:    " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_LUT256_shifted<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256 preshifted ET: " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_LUT256_shifted_ET<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256:               " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_LUT256<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256 ET:            " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_LUT256_ET<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit Magicbits:            " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_magicbits<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit For:                  " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_for<uint_fast32_t>, times) << endl;
	cout << "    32-bit For ET:               " << testEncode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Encode_for_ET<uint_fast32_t>, times) << endl;
}

static void Decode_3D_Perf(){
	cout << "++ Decoding " << MAX << "^3 morton codes in LINEAR order (" << total << " in total)" << endl;
//#if _WIN64 || __x86_64__
	cout << "    64-bit LUT256 preshifted:    " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_LUT256_shifted<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256 preshifted ET: " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_LUT256_shifted_ET<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256:               " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_LUT256<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit LUT256 ET:            " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_LUT256_ET<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit Magicbits:            " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_magicbits<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit For:                  " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_for<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "    64-bit For ET:               " << testDecode_3D_Perf<uint_fast64_t, uint_fast32_t>(&morton3D_Decode_for_ET<uint_fast64_t, uint_fast32_t>, times) << endl;
	cout << "" << endl;
//#else
	cout << "    32-bit LUT256 preshifted:    " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_LUT256_shifted<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256 preshifted ET: " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_LUT256_shifted_ET<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256:               " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_LUT256<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit LUT256 ET:            " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_LUT256_ET<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit Magicbits:            " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_magicbits<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit For:                  " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_for<uint_fast32_t, uint_fast16_t>, times) << endl;
	cout << "    32-bit For ET:               " << testDecode_3D_Perf<uint_fast32_t, uint_fast16_t>(&morton3D_Decode_for_ET<uint_fast32_t, uint_fast16_t>, times) << endl;
//#endif
}

void printHeader(){
	cout << "LIBMORTON TEST SUITE" << endl;
	cout << "--------------------" << endl;
#if _WIN64 || __x86_64__  
	cout << "++ 64-bit version" << endl;
#else
	cout << "++ 32-bit version" << endl;
#endif
#if _MSC_VER
	cout << "++ Compiled using MSVC" << endl;
#elif __GNUC__
    cout << "++ Compiled using GCC" << endl;
#endif
}

int main(int argc, char *argv[]) {
	times = 10;
	printHeader();

	//for (size_t i = 0; i < 100; i++) {
	//	uint_fast32_t x;
	//	uint_fast32_t y;
	//	morton2D_Decode_for<size_t,uint_fast32_t>(i, x, y);

	//	std::bitset<64> ibits(i);
	//	std::bitset<32> xbits(x);
	//	std::bitset<32> ybits(y);
	//	cout << ibits << " " << xbits << " " << ybits << endl;

	//}
	
	// register functions
	//f_64_encode.push_back(encode_3D_64("64-bit encode for", &morton3D_64_Encode_for));

	cout << "++ Checking all methods for correctness" << endl;
	check3D_EncodeCorrectness();
	check3D_DecodeCorrectness();
	cout << "++ Running each performance test " << times << " times and averaging results" << endl;
	for (int i = 128; i <= 512; i = i * 2){
		MAX = i;
		total = MAX*MAX*MAX;
		Encode_3D_Perf();
		Decode_3D_Perf();
		printRunningSums();
	}
}
