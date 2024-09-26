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

#pragma once

#include <sys/types.h>
#include <utils/String16.h>

#include <filesystem>

namespace bert_collector {

const android::String16 kBertDropBoxTag("DesktopFirmwareCrash");

const std::filesystem::path kAcpiTablesPath = "/sys/firmware/acpi/tables";
const std::filesystem::path kBertTablePath = kAcpiTablesPath / "BERT";
const std::filesystem::path kBertDataPath = kAcpiTablesPath / "data" / "BERT";

#define ACPI_NAME_SIZE 4
#define ACPI_SIG_BERT "BERT"
#define ACPI_BERT_REGION_STRUCT_SIZE (5 * sizeof(uint32_t))

// BERT (Boot Error Record Table) as defined in ACPI spec, APEI chapter at
// http://www.uefi.org/sites/default/files/resources/ACPI%206_2_A_Sept29.pdf.
struct acpi_table_bert {
  char signature[ACPI_NAME_SIZE];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  char asl_compiler_id[ACPI_NAME_SIZE];
  uint32_t asl_compiler_revision;
  uint32_t region_length;
  uint64_t address;
};

// Read BERT table and data files. Return false if bert table is unavailable or
// invalid.
bool BertRead(const std::filesystem::path &bert_table_path,
              const std::filesystem::path &bert_data_path,
              struct acpi_table_bert &bert_table,
              std::string &bert_table_contents,
              std::string &bert_data_contents);

// Collect BERT table and data and encode for dumping.
// Return false if bert table is unable to be generated for any reason.
bool CollectReport(
    std::string &report,
    const std::filesystem::path &bert_table_path = kBertTablePath,
    const std::filesystem::path &bert_data_path = kBertDataPath);

// Dump BERT report to DropBox
bool DumpReport(const std::string &report,
                const android::String16 tag = kBertDropBoxTag);

} // namespace bert_collector
