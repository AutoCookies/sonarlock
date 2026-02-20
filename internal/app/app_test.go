package app

import (
	"bytes"
	"strings"
	"testing"
)

func TestRunVersionCommand(t *testing.T) {
	stdout := &bytes.Buffer{}
	stderr := &bytes.Buffer{}

	code := New(stdout, stderr).Run([]string{"version"})
	if code != 0 {
		t.Fatalf("expected exit code 0, got %d (stderr=%q)", code, stderr.String())
	}

	output := stdout.String()
	required := []string{"SnapSync ", "commit:", "built:", "go:", "os/arch:"}
	for _, token := range required {
		if !strings.Contains(output, token) {
			t.Fatalf("expected output to contain %q, got %q", token, output)
		}
	}
}
