#include <iostream>
#include <omp.h>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>

using namespace std;
using namespace chrono;

vector<unsigned int> GenerateValues(unsigned int size)
{
	auto millis = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	default_random_engine e(static_cast<unsigned int>(millis.count()));
	vector<unsigned int> data;
	for (unsigned int i = 0; i < size; i++)
	{
		data.push_back(e());
	}
	return data;
}

void BubbleSort(vector<unsigned int> data)
{
	for (int count = data.size(); count > 2; count--)
	{
		for (int i = 0; i < count - 1; i++)
		{
			if (data[i] > data[i + 1])
			{
				unsigned int temp = data[i];
				data[i] = data[i + 1];
				data[i + 1] = temp;
			}
		}
	}
}

int main()
{
	ofstream results("bubble.csv", ofstream::out);

	for (unsigned int size = 8; size <= 16; size++)
	{
		results << pow(2, size) << ", ";
		for (unsigned int i = 0; i < 100; i++)
		{
			cout << "Generating " << i << " for " << pow(2, size) << " values" << endl;
			auto data = GenerateValues(static_cast<unsigned int>(pow(2, size)));
			cout << "Sorting" << endl;
			auto start = system_clock::now();
			BubbleSort(data);
			auto end = system_clock::now();
			auto total = duration_cast<milliseconds>(end - start).count();
			results << total << ",";
		}
		results << endl;
	}
	results.close();

	return 0;
}