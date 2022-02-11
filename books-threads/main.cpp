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
#include <map>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <thread>

#include <boost/algorithm/string.hpp>

// A structure threads manipulate when doing word calculation
typedef struct thread_struct_t {
   thread_struct_t(int start, int end,
                   const std::vector<std::wstring> & rawWords,
                   const std::vector<std::wstring> & toIgnore)
   : startIndex(start), endIndex(end), rawWords(rawWords), toIgnore(toIgnore) {
      // Empty
   }
   const int startIndex;
   const int endIndex;
   int wordCount;
   int ignoredWordCount;
   const std::vector<std::wstring> & rawWords;
   const std::vector<std::wstring> & toIgnore;
   std::map<std::wstring,int> threadWordCounts;
} thread_struct;

// The thread function
void run(thread_struct & threadData) {
   for (int index = threadData.startIndex; index <= threadData.endIndex; index++) {
      const std::wstring & word = threadData.rawWords[index];
      if (word.length() > 1 && std::find(threadData.toIgnore.begin(), threadData.toIgnore.end(), word) == threadData.toIgnore.end()) {
         threadData.wordCount++;
         threadData.threadWordCounts[word] += 1;
      } else {
         threadData.ignoredWordCount++;
      }
   }
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

   // Start measuring time
   std::chrono::system_clock::time_point started = std::chrono::system_clock::now();

   // This map will contain the unique words with counts
   std::map<std::wstring, int> wordCounts;

   // This array contains the words to ignore
   std::vector<std::wstring> wordsToIgnore;

   // Read ignore words
   std::wifstream ignoreFile(argv[2]);
   ignoreFile.imbue(loc);
   const std::wstring ignoreSeparators(L",\n\r");
   std::wstring line;
   while (std::getline(ignoreFile,line)) {
      std::vector<std::wstring> ignoreWords;
      boost::split(ignoreWords, line, boost::is_any_of(ignoreSeparators));
      wordsToIgnore.insert(wordsToIgnore.end(), ignoreWords.begin(), ignoreWords.end());
   }
   ignoreFile.close();

   // Read book words into words array
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

   // Prepare the threads
   enum { NUM_THREADS = 8 };
   int sliceSize = wordArray.size() / NUM_THREADS;
   int lastSliceSize = sliceSize + wordArray.size() % NUM_THREADS;
   // Thread data is placed in this vector
   thread_struct * threadStructs[NUM_THREADS];
   for (int counter = 0; counter < NUM_THREADS; counter++) {
      int startIndex = counter;
      int endIndex = std::min(counter + sliceSize - 1, (int)wordArray.size() - 1);
      if (counter == NUM_THREADS - 1) {
         endIndex = counter + lastSliceSize - 1;
      }
      threadStructs[counter] = new thread_struct(startIndex, endIndex, wordArray, wordsToIgnore);
   }

   // Launch the threads and put them into the array
   std::thread threads[NUM_THREADS];
   for (int counter = 0; counter < NUM_THREADS; counter++) {
      threads[counter] = std::thread(run, std::ref(*threadStructs[counter]));
   }

   // Wait for the threads to finish
   for (int counter = 0; counter < NUM_THREADS; counter++) {
      threads[counter].join();
   }

   // merge thread results to wordCount and total counters
   int wordsTotal = 0;
   int ignoredWordsTotal = 0;

   for (int counter = 0; counter < NUM_THREADS; counter++) {
      for (const auto wordAndCount : threadStructs[counter]->threadWordCounts) {
         wordCounts[wordAndCount.first] += wordAndCount.second;
         wordsTotal += threadStructs[counter]->wordCount;
         ignoredWordsTotal += threadStructs[counter]->ignoredWordCount;
      }
   }
   for (int counter = 0; counter < NUM_THREADS; counter++) {
      delete threadStructs[counter];
   }

   // Move words & counts to multimap reversed so that it can be sorted by value (count) with operator >
   // Multimap needed since several words can have same count -- which will be the key in the new multimap.
   std::multimap<int,std::wstring, std::greater<int>> result;
   std::transform(wordCounts.cbegin(), wordCounts.cend(), std::inserter(result, result.begin()),
                  [](auto const& p) {
                     return std::make_pair(p.second, p.first);
                  });

   // Print
   int counter = 0;
   for (const auto & element : result) {
      if (counter++ == topListSize) {
         break;
      }
      std::wcout << std::setw(4) << counter << L". " << std::left << std::setw(20) << element.second << L" " << std::right << std::setw(6) << element.first << std::endl;
   }

   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
   std::chrono::milliseconds timeValue = std::chrono::duration_cast<std::chrono::milliseconds>(now-started);
   std::wcout << L"Processed the book in " << timeValue.count() << L" ms." << std::endl;

   return EXIT_SUCCESS;
}
