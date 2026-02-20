package main

import (
	"os"

	"snapsync/internal/app"
)

func main() {
	os.Exit(app.Run(os.Args[1:]))
}
