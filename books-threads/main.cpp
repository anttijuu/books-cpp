//
//  main.cpp
//  books
//
//  Created by Antti Juustila on 6.11.2021.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <thread>

#include <boost/algorithm/string.hpp>

/**
 A structure the threads use when doing word frequency calculation.
 */
typedef struct thread_struct_t {
	/**
	 Initializes the thread data struct for processing when threads launch.
	 @param start The start index to the vector slice this thread will process.
	 @param end The end index to the vector slice this thread will process.
	 @param rawWords All the words of the book in a vector.
	 @param toIgnore The words not counted.
	 */
	thread_struct_t(long start, long end,
						 const std::vector<std::wstring> & rawWords,
						 const std::vector<std::wstring> & toIgnore)
	: startIndex(start), endIndex(end), rawWords(rawWords), toIgnore(toIgnore), wordCount(0), ignoredWordCount(0) {
		// Empty
	}
	/** The start index to the all words vector this thread will process. */
	const long startIndex;
	/** The end index to the all words vector this thread will process. */
	const long endIndex;
	/** The count of words in the vector slice this thread processes. */
	long wordCount;
	/** The count of words ignored while the thread processed the words. */
	long ignoredWordCount;

	const std::vector<std::wstring> & rawWords;
	// Constant reference to the words to ignore, read from the ignore file.
	const std::vector<std::wstring> & toIgnore;
	// The results of one thread: the word - word count pairs for unique words
	// in the array slice processed by one thread. Try both map and unordered_map
	// to see time performance differences, if any.
	std::unordered_map<std::wstring,int> wordCounts;
	// std::map<std::wstring,int> wordCounts;
} thread_struct;

// The thread function, processing a slice of the all words read from the book file.
void run(thread_struct & threadData) {
	// For each word read from the book in this array slice...
	// std::cout << "Starting thread " << std::this_thread::get_id() << std::endl;
	for (long index = threadData.startIndex; index <= threadData.endIndex; index++) {
		// Look up the word.
		const std::wstring & word = threadData.rawWords[index];
		// Increase the word count this thread has processed.
		threadData.wordCount++;
		// If the word length is greater than two chars and it is not one of the words to ignore.
		if (word.length() > 1 && std::find(threadData.toIgnore.begin(), threadData.toIgnore.end(), word) == threadData.toIgnore.end()) {
			// Add the frequency count of the word. If the word is not
			// in the map, it is added there with count of 1.
			threadData.wordCounts[word] += 1;
		} else {
			// Otherwise words was ignored, so increase the ignore count for this thread.
			threadData.ignoredWordCount++;
		}
	}
	// std::cout << "Ending thread " << std::this_thread::get_id() << std::endl;
}


int main(int argc, const char * argv[]) {

	if (argc != 4) {
		std::wcout << L"Usage: " << argv[0] << L" bookfile.txt ignore-file.txt 100" << std::endl;
		return EXIT_FAILURE;
	}
	int topListSize = std::stoi(argv[3]);

	// This is needed to prevent C library to convert strings to narrow
	// instead of C++ on some platforms.
	std::ios_base::sync_with_stdio(false);
	std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);

	// Start measuring time.
	std::chrono::system_clock::time_point started = std::chrono::system_clock::now();

	// This map will contain the unique words with counts. Try both map and unordered_map
	// to see time performance differences, if any.
	std::unordered_map<std::wstring, int> wordCounts;
	// std::map<std::wstring, int> wordCounts;

	// This array contains the words to ignore
	std::vector<std::wstring> wordsToIgnore;

	// Read the words to ignore.
	std::wifstream ignoreFile(argv[2]);
	ignoreFile.imbue(loc);
	const std::wstring ignoreSeparators(L",\n\r");
	std::wstring line;
	while (std::getline(ignoreFile,line)) {
		std::transform(line.begin(), line.end(), line.begin(),
							[](wchar_t c){ return std::tolower(c); });
		std::vector<std::wstring> ignoreWords;
		boost::split(ignoreWords, line, boost::is_any_of(ignoreSeparators));
		wordsToIgnore.insert(wordsToIgnore.end(), ignoreWords.begin(), ignoreWords.end());
	}
	ignoreFile.close();

	// Read all the book words into words array
	std::vector<std::wstring> wordArray;

	std::wifstream bookFile(argv[1]);
	bookFile.imbue(loc);
	while (std::getline(bookFile,line)) {
		std::wstring word;
		for (auto & ch : line) {
			if (std::isalpha(ch, loc)) {
				word += ch;
			} else {
				std::transform(word.begin(), word.end(), word.begin(),
									[](wchar_t c){ return std::tolower(c); });
				wordArray.push_back(word);
				word = L"";
			}
		}
	}
	bookFile.close();

	// Prepare the threads, eight of them (assuming eight cores in the computer).
	enum { NUMBER_OF_THREADS = 8 };
	long sliceSize = wordArray.size() / NUMBER_OF_THREADS;
	long lastSliceSize = sliceSize + wordArray.size() % NUMBER_OF_THREADS;

	// Thread data is placed in this array.
	thread_struct * threadStructs[NUMBER_OF_THREADS];
	long startIndex = 0;
	for (int counter = 0; counter < NUMBER_OF_THREADS; counter++) {
		long endIndex = std::min<long>(startIndex + sliceSize - 1, wordArray.size() - 1);
		if (counter == NUMBER_OF_THREADS - 1) {
			endIndex = startIndex + lastSliceSize - 1;
		}
		threadStructs[counter] = new thread_struct(startIndex, endIndex, wordArray, wordsToIgnore);
		startIndex = endIndex + 1;
	}

	// Launch the threads keeping them in the array...
	std::thread threads[NUMBER_OF_THREADS];
	for (int counter = 0; counter < NUMBER_OF_THREADS; counter++) {
		threads[counter] = std::thread(run, std::ref(*threadStructs[counter]));
	}

	// ...so we can wait for the threads to finish.
	for (int counter = 0; counter < NUMBER_OF_THREADS; counter++) {
		threads[counter].join();
	}

	// Merge thread results from the thread structs to wordCount and total counters.
	long countedWordsTotal = 0;
	long ignoredWordsTotal = 0;

	for (int counter = 0; counter < NUMBER_OF_THREADS; counter++) {
		for (const auto wordAndCount : threadStructs[counter]->wordCounts) {
			wordCounts[wordAndCount.first] += wordAndCount.second;
		}
		countedWordsTotal += threadStructs[counter]->wordCount;
		ignoredWordsTotal += threadStructs[counter]->ignoredWordCount;
	}

	for (int counter = 0; counter < NUMBER_OF_THREADS; counter++) {
		delete threadStructs[counter];
	}

	// Move words & counts to multimap reversed so that it can be sorted by value (count) with operator >
	// Multimap needed since several words can have same count -- which will be the key in the new multimap.
	std::multimap<int, std::wstring, std::greater<int>> result;
	std::transform(wordCounts.cbegin(), wordCounts.cend(), std::inserter(result, result.begin()),
						[](auto const& p) {
		return std::make_pair(p.second, p.first);
	});

	// Print the results of top words with counts.
	int counter = 0;
	for (const auto & element : result) {
		if (counter++ == topListSize) {
			break;
		}
		std::wcout << std::setw(4) << counter << L". " << std::left << std::setw(20) << element.second << L" " << std::right << std::setw(6) << element.first << std::endl;
	}

	// Stop measuring time and print out the time performance.
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::milliseconds timeValue = std::chrono::duration_cast<std::chrono::milliseconds>(now-started);
	std::wcout << L"Processed the book in     " << timeValue.count() << L" ms." << std::endl;
	std::wcout << L"Words in book file:       " << wordArray.size() << std::endl;
	std::wcout << L"Counted words in total:   " << countedWordsTotal << std::endl;
	std::wcout << L"Words to ignore:          " << wordsToIgnore.size() << std::endl;
	std::wcout << L"Words ignored in total:   " << ignoredWordsTotal << std::endl;
	std::wcout << L"Unique words in total:    " << wordCounts.size() << std::endl;

	return EXIT_SUCCESS;
}
