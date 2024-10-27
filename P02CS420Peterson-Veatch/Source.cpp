#include<iostream>
#include<thread>
#include<fstream>
#include<bitset>
#include<vector>
#include<string>
#include<mutex>
#include<algorithm>

using namespace std;

//C++ function to transfer contents of a file into a buffer in RAM
void fileToMemoryTransfer(char* fileName, char** data, size_t& numOfBytes) {
	streampos begin, end;
	ifstream inFile(fileName, ios::in | ios::binary | ios::ate);
	if (!inFile)
	{
		cerr << "Cannot open " << fileName << endl;
		inFile.close();
		exit(1);
	}
	size_t size = inFile.tellg();
	char* buffer = new char[size];
	inFile.seekg(0, ios::beg);
	inFile.read(buffer, size);
	inFile.close();
	*data = buffer;
	numOfBytes = size;
}

//Arthur's Portion
void globalHistogramApproach(const char* data, size_t numOfBytes, vector<long long>& globalHistogram) {
	int threadcount = thread::hardware_concurrency();
	int chars = numOfBytes / threadcount;
	int extra = numOfBytes % threadcount;
	int start = 0;
	int end = 0 + chars;
	mutex m;
	vector<thread> workers;

	for (int t = 1; t <= threadcount; t++) {
		if (t == threadcount) {
			end += extra;
		}

		workers.push_back(thread([start, end, numOfBytes, &globalHistogram, &data, &m]() {
			for (int i = start; i < end; i++) {

				m.lock();
				globalHistogram.at(static_cast<unsigned char>(data[i]))++;
				m.unlock();
			}
		}));

		start = end;
		end = start + chars;
	}

	for_each(workers.begin(), workers.end(), [](thread& t) {t.join(); });
}

//Aayush's Portion
// function to compute local histogram
void localHistogramApproach(const char* data, size_t numOfBytes, std::atomic<long long>* globalHistogram, size_t slots) {

	for (size_t i = 0; i < slots; i++) globalHistogram[i] = 0;

	const size_t numThreads = std::thread::hardware_concurrency();
	vector<std::thread> workers;
	vector<vector<long long> > localHistograms(numThreads, vector<long long>(slots, 0));
	size_t rows = numOfBytes / numThreads;
	size_t extra = numOfBytes % numThreads;
	size_t start = 0;

	for (size_t t = 0; t < numThreads; ++t) {
		size_t end = start + rows + (t == numThreads - 1 ? extra : 0);
		workers.emplace_back([data, start, end, &localHistograms, t]() {
			for (size_t i = start; i < end; ++i) {
				unsigned char byteVal = static_cast<unsigned char>(data[i]);
				localHistograms[t][byteVal]++;
			}
			});
		start = end;
	}

	// joining all threads
	for (auto& t : workers) t.join();

	// combining all the local histograms into a single global histogram
	for (size_t t = 0; t < numThreads; ++t) {
		for (size_t i = 0; i < slots; ++i) {
			globalHistogram[i] += localHistograms[t][i];
		}
	}
}


int main(int argc, char* argv[]) {
	char* buffer = nullptr;
	size_t numOfBytes = 0;
	
	if (argc < 2 || argv[1] == nullptr) {
		cerr << "Filename not provided" << endl;
		cerr << "Usage: " << argv[0] << " <file_name>" << endl;
		return 1;
	}

	vector<long long> histogram = vector<long long int>(256, 0); //Histogram for use in Arthur's Portion
	std::atomic<long long>* globalHistogram = new std::atomic<long long>[256]; //Histogram for use with Aayush's portion
	// Aayush used Atomics and Arthur did not


	fileToMemoryTransfer(argv[1], &buffer, numOfBytes);
	
	clock_t start = clock();
	globalHistogramApproach(buffer, numOfBytes, histogram);
	clock_t end = clock();

	cout << "Global Histogram Approach: " << endl;
	cout << "Time to complete: " << (double(end) - double(start)) / CLOCKS_PER_SEC << endl;
	for (int i = 0; i < histogram.size(); i++) { //Print resulting Histogram
		cout << i << ": " << histogram.at(i) << endl;
	}

	start = clock();
	localHistogramApproach(buffer, numOfBytes, globalHistogram, 256);
	end = clock();

	cout << "Local Histogram Approach: " << endl;
	cout << "Time to complete: " << (double(end) - double(start)) / CLOCKS_PER_SEC << endl;
	for (int i = 0; i < 256; i++) { //Print resulting Histogram
		cout << i << ": " << globalHistogram[i] << endl;
	}

	delete[] buffer;
	buffer = nullptr;
	delete[] globalHistogram;
	globalHistogram = nullptr;


	

	return 0;
};