package main

import (
   "fmt"
    "github.com/parquet-go/parquet-go"
)

type RowType struct { Text string }


func main() {
   fmt.Println("Hello World!")

	if err := parquet.WriteFile("simple.parquet", []RowType{
			{Text: "text1"},
			{Text: "text2"},
			{Text: "text3"},
			{Text: "text4"},
			{Text: "text5"},
	}); err != nil {
		fmt.Println("Error writing file:", err)
	}
}
