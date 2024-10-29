/*
 * Copyright 2024, The ChromiumOS Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Including file under test so internal functions can remain static.
// This technique is described in
// https://google.github.io/googletest/advanced.html#testing-private-code
#include "bert_collector.cpp"

#include <brillo/strings/string_utils.h>
#include <gtest/gtest.h>

using std::string_literals::operator""s;

namespace bert_collector {

class BERTCollectorTest : public ::testing::Test {
protected:
  const TemporaryDir tmp_dir_;
  std::filesystem::path test_bert_table_path_;
  std::filesystem::path test_bert_data_path_;

  enum BertTestDataType {
    BAD_DATA,
    CONSTRUCTED_DATA,
    ORGANIC_DATA,
  };

  void PrepareBertDataTest(enum BertTestDataType data_type) {
    std::string test_bert_table;
    std::string test_bert_data;

    switch (data_type) {
    case BAD_DATA:
      test_bert_table = "Bad Bert Table";
      test_bert_data = "Bad Bert Data";
      break;
    case ORGANIC_DATA:
      test_bert_table =
          "\x42\x45\x52\x54\x30\x00\x00\x00\x01\x22\x43\x4f\x52\x45\x76\x34"s
          "\x43\x4f\x52\x45\x42\x4f\x4f\x54\x00\x00\x00\x00\x43\x4f\x52\x45"s
          "\x28\x06\x23\x20\xf0\x00\x00\x00\x00\xc0\x90\x76\x00\x00\x00\x00"s;
      test_bert_data =
          "\x21\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xdc\x00\x00\x00"s
          "\x01\x00\x00\x00\x96\x2a\x21\x81\xed\x09\x96\x49\x94\x71\x8d\x72"s
          "\x9c\x8e\x69\xed\x01\x00\x00\x00\x00\x03\x04\x00\x20\x00\x00\x00"s
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s
          "\x00\x00\x00\x00\x47\x41\x14\x01\x12\x09\x24\x20\x02\x02\x00\x00"s
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11\xf3\x87\x8f"s
          "\x98\xc9\x9e\x4d\xa0\xc4\x60\x65\x51\x8c\x4f\x6d\x96\x2a\x21\x81"s
          "\xed\x09\x96\x49\x94\x71\x8d\x72\x9c\x8e\x69\xed\x01\x00\x00\x00"s
          "\x00\x03\x04\x00\x2c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x47\x41\x14\x01"s
          "\x12\x09\x24\x20\x02\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s
          "\x00\x00\x00\x00\x11\xf3\x87\x8f\x98\xc9\x9e\x4d\xa0\xc4\x60\x65"s
          "\x51\x8c\x4f\x6d\x48\x65\x6c\x6c\x6f\x20\x45\x72\x72\x6f\x72\x21"s;
      break;
    case CONSTRUCTED_DATA:
      // TODO(b/368163368): Construct valid BERT data
      test_bert_data = "TODO: Construct BERT data";
      const struct acpi_table_bert constructed_bert_table = {
          .signature = {'B', 'E', 'R', 'T'},
          .length = sizeof(constructed_bert_table),
          .revision = 'A',
          .checksum = 'D',
          .oem_id = "OEMID",
          .oem_table_id = "TABLEID",
          .oem_revision = 0xFFFFFFFF,
          .asl_compiler_id = "ACP",
          .asl_compiler_revision = 0xEEEEEEEE,
          .region_length = (uint32_t)test_bert_data.size(),
          .address = 0x000000000001234,
      };
      test_bert_table =
          std::string(reinterpret_cast<const char *>(&constructed_bert_table),
                      sizeof(constructed_bert_table));
      break;
    }
    ASSERT_TRUE(android::base::WriteStringToFile(test_bert_table,
                                                 test_bert_table_path_));
    ASSERT_TRUE(
        android::base::WriteStringToFile(test_bert_data, test_bert_data_path_));
  }

public:
  void SetUp() override {
    std::filesystem::path tmp_dir_path(tmp_dir_.path);
    ASSERT_TRUE(std::filesystem::create_directory(tmp_dir_path / "data"));
    test_bert_table_path_ = std::filesystem::path(tmp_dir_path / "BERT");
    test_bert_data_path_ = tmp_dir_path / "data" / "BERT";
  }
};

TEST_F(BERTCollectorTest, TestNoBERTData) {
  std::string report;
  ASSERT_FALSE(
      CollectReport(report, test_bert_table_path_, test_bert_data_path_));
}

TEST_F(BERTCollectorTest, TestBadBERTData) {
  std::string report;
  PrepareBertDataTest(BAD_DATA);
  ASSERT_FALSE(
      CollectReport(report, test_bert_table_path_, test_bert_data_path_));
}

TEST_F(BERTCollectorTest, TestConstructedBERTData) {
  std::string bert_table_contents;
  std::string bert_data_contents;
  struct bert_collector::acpi_table_bert bert_table;
  PrepareBertDataTest(CONSTRUCTED_DATA);
  ASSERT_TRUE(BertRead(test_bert_table_path_, test_bert_data_path_, bert_table,
                       bert_table_contents, bert_data_contents));
  ASSERT_EQ(bert_table.length, sizeof(bert_table));
  ASSERT_EQ(bert_table.length, bert_table_contents.size());
  ASSERT_EQ(bert_table.region_length, bert_data_contents.size());
}

TEST_F(BERTCollectorTest, TestOrganicBERTData) {
  std::string bert_table_contents;
  std::string bert_data_contents;
  struct bert_collector::acpi_table_bert bert_table;
  PrepareBertDataTest(ORGANIC_DATA);
  ASSERT_TRUE(BertRead(test_bert_table_path_, test_bert_data_path_, bert_table,
                       bert_table_contents, bert_data_contents));
  ASSERT_EQ(bert_table.length, sizeof(bert_table));
  ASSERT_EQ(bert_table.length, bert_table_contents.size());
  ASSERT_EQ(bert_table.region_length, bert_data_contents.size());
}

TEST_F(BERTCollectorTest, TestEscapeDelimiter) {
  std::string text = "foo===bar\n"
                     "===foobar===\n"
                     "==foobar\n"
                     "====foobar\n";
  std::string expected = "foo===bar\n"
                         "====foobar===\n"
                         "==foobar\n"
                         "=====foobar\n";
  ASSERT_EQ(expected, EscapeDelmiter(text));
}

TEST_F(BERTCollectorTest, TestBERTReportHeader) {
  std::string header = GetReportHeader();
  ASSERT_FALSE(header.empty());
  ASSERT_TRUE(header.ends_with("\n\n"));

  bool found_subsystem = false;
  for (std::string line : brillo::string_utils::Split(header, "\n")) {
    std::cout << "Header Line: " << std::endl << line << std::endl;
    std::regex header_line_re("^([^:]+): (.*)$", std::regex::multiline);
    std::smatch header_line_match;
    ASSERT_TRUE(std::regex_search(line, header_line_match, header_line_re));
    ASSERT_EQ(header_line_match.size(), 3ul);
    if (header_line_match[1] == "Subsystem") {
      ASSERT_EQ(header_line_match[2], kSubsystem);
      found_subsystem = true;
    }
  }
  ASSERT_TRUE(found_subsystem);
}

TEST_F(BERTCollectorTest, TestDecodeReport) {
  std::string bert_table_contents;
  std::string bert_data_contents;
  struct bert_collector::acpi_table_bert bert_table;
  PrepareBertDataTest(ORGANIC_DATA);
  ASSERT_TRUE(BertRead(test_bert_table_path_, test_bert_data_path_, bert_table,
                       bert_table_contents, bert_data_contents));

  std::string report;
  ASSERT_TRUE(
      CollectReport(report, test_bert_table_path_, test_bert_data_path_));

  std::pair<std::string, std::string> header_body =
      brillo::string_utils::SplitAtFirst(report, "\n\n");
  std::string body = header_body.second;

  // Extract entries from report
  std::regex entry_re("===([^=]+)===\n((?:\n|.)*?)(?=(?:\n===[^=]|$))");
  std::smatch entry_match;

  // Decode BERT table from report
  ASSERT_TRUE(std::regex_search(body, entry_match, entry_re));
  ASSERT_EQ(entry_match.size(), 3ul);
  std::string table_entry_name = entry_match[1].str();
  std::string table_encoded = entry_match[2].str();
  ASSERT_EQ(table_entry_name, test_bert_table_path_);
  std::string table_decoded;
  ASSERT_TRUE(
      brillo::data_encoding::Base64Decode(table_encoded, &table_decoded));
  ASSERT_EQ(table_decoded, bert_table_contents);

  // Decode BERT data from report
  body = entry_match.suffix();
  ASSERT_TRUE(std::regex_search(body, entry_match, entry_re));
  ASSERT_EQ(entry_match.size(), 3ul);
  std::string data_entry_name = entry_match[1].str();
  std::string data_encoded = entry_match[2].str();
  ASSERT_EQ(data_entry_name, test_bert_data_path_);
  std::string data_decoded;
  ASSERT_TRUE(brillo::data_encoding::Base64Decode(data_encoded, &data_decoded));
  ASSERT_EQ(data_decoded, bert_data_contents);
}

} // namespace bert_collector
