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

/*
 * bert_collector collects ACPI BERT table and data if it exists and sends it to
 * DropBoxManager. The BERT table is only created when there is a critical error
 * detected by the BIOS during boot.
 *
 * This implementation was adapted for Android from the ChromOS version:
 * https://source.chromium.org/chromiumos/chromiumos/codesearch/+/main:src/platform2/crash-reporter/bert_collector.cc
 */

#include "bert_collector.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <android/os/DropBoxManager.h>
#include <brillo/data_encoding.h>

#if defined(__BIG_ENDIAN__) ||                                                 \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#error "bert_collector does not support big endian"
#endif

namespace bert_collector {

// Validate BERT table signature, length and region length.
static bool BertCheckTable(const struct acpi_table_bert &bert_table) {
  if (memcmp(bert_table.signature, ACPI_SIG_BERT, ACPI_NAME_SIZE) != 0)
    return false;
  if (bert_table.length != sizeof(struct acpi_table_bert))
    return false;
  if (bert_table.region_length != 0 &&
      bert_table.region_length < ACPI_BERT_REGION_STRUCT_SIZE)
    return false;

  return true;
}

// Read BERT table and data files.
bool BertRead(const std::filesystem::path &bert_table_path,
              const std::filesystem::path &bert_data_path,
              struct acpi_table_bert &bert_table,
              std::string &bert_table_contents,
              std::string &bert_data_contents) {
  // Read BERT table file.
  if (!android::base::ReadFileToString(bert_table_path, &bert_table_contents)) {
    LOG(ERROR) << "Failed to read BERT table at " << bert_table_path;
    return false;
  }
  if (bert_table_contents.length() != sizeof(bert_table)) {
    LOG(ERROR) << std::format("BERT table length is {}, expected {}",
                              bert_table_contents.length(), sizeof(bert_table));
    return false;
  }
  memcpy(&bert_table, bert_table_contents.data(), sizeof(bert_table));

  if (!BertCheckTable(bert_table)) {
    LOG(ERROR) << "Bad data in BERT table";
    return false;
  }

  // Read BERT data file.
  if (!android::base::ReadFileToString(bert_data_path, &bert_data_contents)) {
    LOG(ERROR) << "Failed to read BERT data at " << bert_data_path;
    return false;
  }

  return true;
}

// Default value for unknown header values
const std::string kUnknown = "UNKNOWN";

static std::string GetOnePropertyHeader(const std::string &header_key,
                                        const std::string &property_name) {
  return header_key + ": " +
         android::base::GetProperty(property_name, kUnknown) + "\n";
};

static std::string GetReportHeader() {
  std::string ret = GetOnePropertyHeader("Build", "ro.build.fingerprint");
  ret += GetOnePropertyHeader("Hardware", "ro.product.device");
  ret += GetOnePropertyHeader("Revision", "ro.revision");

  std::string version;
  if (!android::base::ReadFileToString("/proc/version", &version))
    version = kUnknown;
  ret += "Kernel: " + android::base::Trim(version) + "\n";
  return ret;
}

bool CollectReport(std::string &report,
                   const std::filesystem::path &bert_table_path,
                   const std::filesystem::path &bert_data_path) {
  if (!std::filesystem::exists(bert_table_path)) {
    LOG(INFO) << "No BERT table found";
    return false;
  }

  if (!std::filesystem::exists(bert_data_path)) {
    LOG(ERROR) << "BERT data missing";
    return false;
  }

  LOG(INFO) << "BERT error from previous boot (handling)";

  std::string bert_table_contents;
  std::string bert_data_contents;
  struct acpi_table_bert bert_table;
  if (!BertRead(bert_table_path, bert_data_path, bert_table,
                bert_table_contents, bert_data_contents)) {
    return false;
  }
  const std::string bert_table_contents_encoded =
      brillo::data_encoding::Base64Encode(bert_table_contents);
  report = std::format("{}: {}\n", bert_table_path.string(),
                       bert_table_contents_encoded);
  const std::string bert_data_contents_encoded =
      brillo::data_encoding::Base64Encode(bert_data_contents);
  report += std::format("{}: {}\n", bert_data_path.string(),
                        bert_data_contents_encoded);
  return true;
}

bool DumpReport(const std::string report, const android::String16 tag) {
  std::string header = GetReportHeader();
  android::sp<android::os::DropBoxManager> dropbox(
      new android::os::DropBoxManager());
  android::binder::Status status =
      dropbox->addText(tag, header + "\n" + report);
  if (!status.isOk()) {
    LOG(ERROR) << "Failed to write " << tag
               << " to DropBox: " << status.exceptionMessage();
    return false;
  }
  LOG(INFO) << tag << " sent to DropBox";

  return true;
}

} // namespace bert_collector
