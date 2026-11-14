// The values of the three workloads of bench/render.cpp, as plain structs.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace probe {

struct Simple {
  std::string title;
  std::string body;
};

struct Stock {
  std::string symbol;
  std::string url;
  std::string name;
  std::string price;
  std::string change;
  std::string ratio;
};

struct Item {
  std::string rowClass;
  std::string index;
  std::string negativeClass;
  Stock       value;
};

struct Tag {
  std::string name;
};

struct Article {
  std::string      title;
  std::string      url;
  std::string      author;
  std::string      date;
  std::string      teaser;
  std::vector<Tag> tags;
};

struct News {
  std::string          title;
  std::vector<Article> articles;
};

inline Simple
simple()
{
  return {"Hello, Ramhorns!", "This is a really simple test of the rendering!"};
}

inline std::vector<Item>
stock_items()
{
  const std::vector<Stock> stocks = {
        {"ADBE", "http://www.adobe.com", "Adobe Systems", "39.26", "0.13", "0.33"},
        {"AMD", "http://www.amd.com", "Advanced Micro Devices", "16.22", "0.17", "1.06"},
        {"AMZN", "http://www.amazon.com", "Amazon.com", "36.85", "-0.23", "-0.62"},
        {"AAPL", "http://www.apple.com", "Apple", "85.38", "-0.87", "-1.01"},
        {"BEAS", "http://www.bea.com", "BEA Systems", "12.46", "0.09", "0.73"},
        {"CA", "http://www.ca.com", "CA", "24.66", "0.38", "1.57"},
        {"CSCO", "http://www.cisco.com", "Cisco Systems", "26.35", "0.13", "0.5"},
        {"DELL", "http://www.dell.com/", "Dell", "23.73", "-0.42", "-1.74"},
        {"EBAY", "http://www.ebay.com", "eBay", "31.65", "-0.8", "-2.47"},
        {"GOOG", "http://www.google.com", "Google", "495.84", "7.75", "1.59"},
        {"HPQ", "http://www.hp.com", "Hewlett-Packard", "41.69", "-0.02", "-0.05"},
        {"IBM", "http://www.ibm.com", "IBM", "97.45", "-0.06", "-0.06"},
        {"INTC", "http://www.intel.com", "Intel", "20.53", "-0.07", "-0.34"},
        {"JNPR", "http://www.juniper.net/", "Juniper Networks", "18.96", "0.5", "2.71"},
        {"MSFT", "http://www.microsoft.com", "Microsoft", "30.6", "0.15", "0.49"},
        {"ORCL", "http://www.oracle.com", "Oracle", "17.15", "0.17", "1.1"},
        {"SAP", "http://www.sap.com", "SAP", "46.2", "-0.16", "-0.35"},
        {"STX", "http://www.seagate.com/", "Seagate Technology", "27.35", "-0.36", "-1.3"},
        {"SUNW", "http://www.sun.com", "Sun Microsystems", "6.33", "-0.01", "-0.16"},
        {"YHOO", "http://www.yahoo.com", "Yahoo", "28.04", "-0.17", "-0.6"},
  };
  std::vector<Item> items;
  for (std::size_t i = 0; i < stocks.size(); i++) {
    const bool up = stocks.at(i).change.front() != '-' && stocks.at(i).change != "0";
    items.push_back(Item{(i + 1) % 2 == 0 ? "even" : "odd", std::to_string(i + 1), up ? "" : "class=\"minus\"", stocks.at(i)});
  }
  return items;
}

inline News
news()
{
  News d{"Today's news", {}};
  for (int i = 0; i < 20; i++) {
    const std::string n = std::to_string(i + 1);
    d.articles.push_back(Article{"Article " + n + ": what happened & why",
                                 "https://news.example/articles/" + n,
                                 "Author " + n,
                                 "2026-09-" + std::string(i < 9 ? "0" : "") + n,
                                 "A teaser of about one line for article " + n + ", with <b>markup</b> to escape.",
                                 {{"world"}, {"tech"}, {"tag-" + n}}});
  }
  return d;
}

}
