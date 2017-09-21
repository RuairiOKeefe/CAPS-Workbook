#include <thread>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>

using namespace std;
using namespace std::chrono;

void MonteCarlo(unsigned int iterations)
{
	auto millis = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	default_random_engine e(millis.count());
	uniform_real_distribution<double> distribution(0.0, 1.0);
	unsigned int inCircle = 0;
	for (unsigned int i = 0; i < iterations; i++)
	{
		auto x = distribution(e);
		auto y = distribution(e);
		auto length = sqrt((x*x) + (y*y));
		if (length <= 1.0)
			inCircle++;
	}
	auto pi = (4.0*inCircle) / static_cast<double>(iterations);
}

int main()
{
	ofstream data("montecarlo.csv", ofstream::out);
	for (unsigned int numThreads = 0; numThreads <= 6; numThreads++)
	{
		auto totalThreads = static_cast<unsigned int>(pow(2.0, numThreads));
		cout << "Number of threads: " << totalThreads << endl;
		data << "numThreads " << totalThreads;
		for (unsigned int iters = 0; iters < 100; iters++)
		{
			auto start = system_clock::now();
			vector<thread> threads;
			for (unsigned int n = 0; n < totalThreads; n++)
			{
				threads.push_back(thread(MonteCarlo, static_cast<unsigned int> (pow(2.0, 24.0 - numThreads))));
			}
			for (auto &t : threads)
			{
				t.join();
			}
			auto end = system_clock::now();
			auto total = end - start;
			data << ", " << duration_cast<milliseconds>(total).count();
		}
		data << endl;
	}
	data.close();
	return 0;
}