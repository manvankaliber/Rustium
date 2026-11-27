#include "net/disk_cache/simple/disk_simple_index_rust_original_file.h"

#include "base/check.h"
#include "net/disk_cache/disk_cache.h"
namespace disk_cache {
OriginalSimpleIndexLoadResult::OriginalSimpleIndexLoadResult() = default;
OriginalSimpleIndexLoadResult::~OriginalSimpleIndexLoadResult() = default;
OriginalSimpleIndexFile::OriginalSimpleIndexFile(
    scoped_refptr<base::SequencedTaskRunner> runner,
    scoped_refptr<BackendFileOperationsFactory> factory,
    net::CacheType type,
    const base::FilePath& dir)
    : cache_runner_(std::move(runner)),
      file_operations_factory_(std::move(factory)),
      cache_type_(type),
      cache_directory_(dir),
      index_file_(dir.AppendASCII("index-dir").AppendASCII("the-real-index")),
      temp_index_file_(dir.AppendASCII("index-dir").AppendASCII("temp-index")) {
}
OriginalSimpleIndexFile::~OriginalSimpleIndexFile() = default;
void OriginalSimpleIndexFile::LoadIndexEntries(base::Time,
                                               base::OnceClosure,
                                               OriginalSimpleIndexLoadResult*) {
  CHECK(false);
}
void OriginalSimpleIndexFile::WriteToDisk(
    net::CacheType,
    OriginalSimpleIndex::IndexWriteToDiskReason,
    const OriginalSimpleIndex::EntrySet&,
    uint64_t,
    base::OnceClosure) {
  CHECK(false);
}
}  // namespace disk_cache
