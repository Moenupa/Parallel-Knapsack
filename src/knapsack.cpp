/**
 * @author Meng Wang
 * @email mwang106@ur.rochester.edu
 * @create date 2024-02-22 15:24
 * @modify date 2024-02-23 19:56
 * @desc Assignment 1: Distributed Knapsack Problem, Parallel and Distributed Systems CSC458 SPRING 2024
 */

#include <iostream>
#include <stdlib.h>
#include <chrono>

#include <barrier>
#include <syncstream>
#include <thread>
#include <vector>

using namespace std;

int *weights;
int *values;
int **opt;

int knapsack(int n, int curr, int c)
{
	/*find the optimal value of the knapsack with
	items curr to n, capacity c, and weights and values given
	*/

	// base case, we are on the last item
	if (curr == n - 1)
	{
		if (weights[curr] <= c)
		{
			// take if we can
			return values[curr];
		}
		else
		{
			// leave otherwise
			return 0;
		}
	}

	if (opt[curr][c] != -1)
	{
		// value already computed
		return opt[curr][c];
	}
	// need to compute the value

	// optimal value not taking item curr
	int leave = knapsack(n, curr + 1, c);
	int take = -1;

	if (c >= weights[curr])
	{
		// take the item, reducing capacity, but gaining value
		take = knapsack(n, curr + 1, c - weights[curr]) + values[curr];
	}

	if (leave > take)
	{
		opt[curr][c] = leave;
	}
	else
	{
		opt[curr][c] = take;
	}
	return opt[curr][c];
}

int knapsack_parallel(int n, int capacity, int n_threads)
{
	barrier<> sync_point(n_threads);

	// solves range [l, r) for all n items,
	// after each row is done, hit barrier to sync between threads
	auto knapsack_worker = [&](int l, int r)
	{
		int *curr = opt[1];
		int *prev = opt[0];

		// initialize the first row
		for (int col = l; col < r; col++)
			curr[col] = (col < weights[0]) ? 0 : values[0];

		// wait first row filled
		sync_point.arrive_and_wait();

		// iterate through second-to-last rows
		// and wait for the previous row to be filled before updating current row
		for (int row = 1; row < n; row++)
		{
			swap(curr, prev);
			// iterate through in the capcity range
			// update the current row
			for (int col = l; col < r; col++)
			{
				// no enough space to take
				if (col < weights[row])
					curr[col] = prev[col];

				// give or take the item, and max
				else
					curr[col] = max(
						prev[col],
						prev[col - weights[row]] + values[row]);
			}

			// for debugging
			// osyncstream(cout) << this_thread::get_id() << " iter=" << row 
			// << " [" << l << ", " << r << ")" << endl;
			sync_point.arrive_and_wait();
		}
	};

	// distribute the work to n_threads
	int jobs = (capacity + 1) / n_threads;
	int bound = jobs + (capacity + 1) % n_threads;

	// create threads, assign work
	// first thread takes all remainders
	vector<thread> threads;
	threads.reserve(n_threads); 
	threads.emplace_back(knapsack_worker, 0, bound);
	for (; bound <= capacity; bound += jobs)
	{
		threads.emplace_back(knapsack_worker, bound, bound + jobs);
	}

	for (auto &thread : threads)
		thread.join();

	return opt[n & 1][capacity];
}

int main(int argc, char *argv[])
{
	using chrono::duration;
	using chrono::high_resolution_clock;

	// parsing input argument: number of threads
	// since serial version does not have it, there may be diff in time
	// we do this explicitly before the clock starts
	int n_threads = 0;
	if (argc > 1)
	{
		n_threads = stoi(argv[1]);
	}
	cout << "Number of threads: " << n_threads << "." << endl;

	// start the clock
	auto start = high_resolution_clock::now();

	int n; // number of items in the knapsack
	int c; // capacity of the sack
	cin >> n >> c;
	weights = new int[n]; // array to store the weights of the items
	values = new int[n];  // array to store the values of the items
	for (int i = 0; i < n; i++)
	{
		cin >> weights[i] >> values[i];
	}

	int highest;
	if (n_threads > 0)
	{
		opt = new int *[2]
		{ new int[c + 1], new int[c + 1] };
		n_threads = min(n_threads, c + 1);
		highest = knapsack_parallel(n, c, n_threads);
	}
	else
	{
		// initialize the array for dynamic programming
		opt = new int *[n];
		for (int i = 0; i < n; i++)
		{
			opt[i] = new int[c + 1];
			// intialize to -1
			for (int j = 0; j < c + 1; j++)
			{
				opt[i][j] = -1;
			}
		}
		highest = knapsack(n, 0, c);
	}

	auto end = high_resolution_clock::now();
	duration<double, milli> time = end - start;

	cout << "The maximum value is " << highest << "." << endl;
	cout << "Duration: " << time.count() << " miliseconds." << endl;
}