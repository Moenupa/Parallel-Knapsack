/**
 * @author Andrew Read-McFarland, Meng Wang
 * @email mwang106@ur.rochester.edu
 * @create date 2024-02-22 15:24:34
 * @modify date 2024-02-22 15:24:34
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

int knapsack_parallel(int n, int capacity, int num_threads)
{
	// distribute the work
	int jobs = (capacity + 1) / num_threads;

	vector<thread> threads;
	threads.reserve(num_threads);

	std::barrier sync_point(num_threads);

	// solves range [l, r) for all n items, 
	// after each row is done, hit barrier to sync between threads
	auto knapsack_worker = [&](int l, int r)
	{
		int *prev;
		int *curr = opt[0];

		// initialize the first row
		for (int col = l; col < r; col++)
		{
			curr[col] = (col < weights[0]) ? 0 : values[0];
		}
		sync_point.arrive_and_wait();

		// iterate through second-to-last rows
		for (int row = 1; row < n; row++)
		{
			// get the current and previous row
			prev = curr;
			curr = opt[row];

			// iterate through in the capcity range
			// update the current row
			for (int col = l; col < r; col++)
			{
				// no enough space to take
				if (col < weights[row])
					curr[col] = prev[col];

				// give or take the item: curr[col] = max(prev[col], prev[col-w] + v)
				else
					curr[col] = max(prev[col], prev[col - weights[row]] + values[row]);
			}

			// for debugging
			// osyncstream(cout) << this_thread::get_id() << " iter=" << i << " [" << l << ", " << r << ")" << time << endl;
			sync_point.arrive_and_wait();
		}
	};

	int l = 0, r = jobs + (capacity + 1) % num_threads;
	threads.emplace_back(knapsack_worker, l, r);
	for (int t = 1; t < num_threads; t++)
	{
		l = r;
		r = l + jobs;
		threads.emplace_back(knapsack_worker, l, r);
	}

	// wait for all threads to finish
	for (int t = 0; t < num_threads; t++)
	{
		threads[t].join();
	}

	return opt[n-1][capacity];
}

int main(int argc, char *argv[])
{
	using chrono::duration;
	using chrono::high_resolution_clock;

	// parsing input argument: number of threads
	// since serial version does not have it, there may be diff in time
	// we do this explicitly before the clock starts
	int num_threads = 0;
	if (argc > 1)
	{
		num_threads = atoi(argv[1]);
	}
	cout << "Number of threads: " << num_threads << "." << endl;

	// start the clock
	auto start = high_resolution_clock::now();

	int n; // number of items in the knapsack
	cin >> n;
	int c; // capacity of the sack
	cin >> c;
	weights = new int[n]; // array to store the weights of the items
	values = new int[n];  // array to store the values of the items
	for (int i = 0; i < n; i++)
	{
		cin >> weights[i];
		cin >> values[i];
	}

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

	int highest;
	if (num_threads > 0)
	{
		highest = knapsack_parallel(n, c, num_threads);
	}
	else
	{
		highest = knapsack(n, 0, c);
	}

	auto end = high_resolution_clock::now();
	duration<double, milli> time = end - start;

	cout << "The maximum value is " << highest << "." << endl;
	cout << "Duration: " << time.count() << " miliseconds." << endl;
}