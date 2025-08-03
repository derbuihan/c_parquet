```bash
# Download the Parquet Thrift definition file
wget https://raw.githubusercontent.com/apache/parquet-format/refs/heads/master/src/main/thrift/parquet.thrift

# Generate C code from the Thrift definition
thrift --gen c_glib parquet.thrift
```
