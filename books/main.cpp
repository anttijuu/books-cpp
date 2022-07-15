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
#include <unordered_map>
#include <locale>
#include <codecvt>

#include <boost/algorithm/string.hpp>

int main(int argc, const char * argv[]) {

   if (argc != 4) {
      std::wcout << L"Usage: " << argv[0] << L" bookfile.txt ignore-file.txt 100" << std::endl;
      return EXIT_FAILURE;
   }
   int topListSize = std::stoi(argv[3]);

   // This is needed to prevent C library to convert strings to narrow
   // instead of C++ on some platforms.
   std::ios_base::sync_with_stdio(false);

   // Start measuring time performance.
   std::chrono::system_clock::time_point started = std::chrono::system_clock::now();

   // This dictionary will contain the words with counts.
   // std::map<std::wstring, int> wordCount;
	std::unordered_map<std::wstring, int> wordCount;

   // Line of text read from a file.
   std::wstring line;

   // The words to ignore; not to be counted.
   std::vector<std::wstring> wordsToIgnore;
   const std::wstring ignoreSeparators(L",\n\r");

   // Use wide chars, assuming file is utf-8.
   std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);

   // Read ignore words
   std::wifstream ignoreFile(argv[2]);
   ignoreFile.imbue(loc);
   while (std::getline(ignoreFile,line)) {
      // Convert line to lowecase.
      std::transform(line.begin(), line.end(), line.begin(),
                     [](wchar_t c){ return std::tolower(c); });
      std::vector<std::wstring> ignoreWords;
      boost::split(ignoreWords, line, boost::is_any_of(ignoreSeparators));
      wordsToIgnore.insert(wordsToIgnore.end(), ignoreWords.begin(), ignoreWords.end());
   }
   ignoreFile.close();

   int ignoredWordsTotal = 0;
   int wordsInTotal = 0;

   // Read book words, line by line.
   int totalWordCount = 0;
   int ignoredWordCount = 0;
   int countedWords = 0;
   std::wifstream bookFile(argv[1]);
   bookFile.imbue(loc);
   while (std::getline(bookFile,line)) {
      std::wstring word;
      for (auto & ch : line) {
         // Words separated by chars not alpha.
         if (std::isalpha(ch, loc)) {
            word += ch;
         } else {
            totalWordCount++;
            if (word.length() > 1) {
               std::transform(word.begin(), word.end(), word.begin(),
                              [](wchar_t c){ return std::tolower(c); });
               if (std::find(wordsToIgnore.begin(), wordsToIgnore.end(), word) == wordsToIgnore.end()) {
                  countedWords++;
                  wordCount[word] += 1;
               } else {
                  ignoredWordCount++;
               }
            } else {
               ignoredWordCount++;
            }
            word = L"";
         }
      }
   }
   bookFile.close();

   int uniqueWordsUseCount = 0;
   // Move words & counts to multimap reversed so that it can be sorted by value (count) with operator >
   // Multimap needed since several words can have same count -- which will be the key in the new multimap.
   std::multimap<int,std::wstring, std::greater<int>> result;
   std::transform(wordCount.cbegin(), wordCount.cend(), std::inserter(result, result.begin()),
                  [&uniqueWordsUseCount](auto const& p) {
                     uniqueWordsUseCount += p.second;
                     return std::make_pair(p.second, p.first);
                  });

   // Print
   int counter = 0;
   for (const auto & element : result) {
      if (counter++ == topListSize) {
         break;
      }
      std::wcout << std::setw(4) << counter << ". " << std::left << std::setw(20) << element.second << " " << std::right << std::setw(6) << element.first << std::endl;
   }

   // Stop counting time and report the performance.
   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
   std::chrono::milliseconds timeValue = std::chrono::duration_cast<std::chrono::milliseconds>(now-started);
   std::wcout << L"Processed the book in " << timeValue.count() << L" ms." << std::endl;
   std::wcout << L"Words in book file:       " << totalWordCount << std::endl;
   std::wcout << L"Counted words in total:   " << countedWords << std::endl;
   std::wcout << L"Words to ignore:          " << wordsToIgnore.size() << std::endl;
   std::wcout << L"Words ignored in total:   " << ignoredWordCount << std::endl;
   std::wcout << L"Unique words in total:    " << wordCount.size() << std::endl;

   return EXIT_SUCCESS;
}
