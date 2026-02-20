package cli

import (
	"bytes"
	"strings"
	"testing"
)

func TestRootCommandIncludesPhaseOneSubcommands(t *testing.T) {
	root := RootCommand(&bytes.Buffer{}, strings.NewReader(""))
	if root == nil {
		t.Fatal("expected root command")
	}

	names := map[string]bool{}
	for _, sub := range root.Subcommands {
		names[sub.Name] = true
	}
	for _, required := range []string{"version", "send", "recv"} {
		if !names[required] {
			t.Fatalf("expected %q subcommand", required)
		}
	}
}

func TestExecuteHelpReturnsSuccess(t *testing.T) {
	out := &bytes.Buffer{}
	root := RootCommand(out, strings.NewReader(""))

	err := Execute(root, []string{"--help"}, out)
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}

	if !strings.Contains(out.String(), "Usage:") {
		t.Fatalf("expected help output, got %q", out.String())
	}
}
