package cli

import (
	"bytes"
	"strings"
	"testing"
)

func TestRootCommandIncludesVersionSubcommand(t *testing.T) {
	root := RootCommand(&bytes.Buffer{})
	if root == nil {
		t.Fatal("expected root command")
	}

	found := false
	for _, sub := range root.Subcommands {
		if sub.Name == "version" {
			found = true
			break
		}
	}
	if !found {
		t.Fatal("expected version subcommand")
	}
}

func TestExecuteHelpReturnsSuccess(t *testing.T) {
	out := &bytes.Buffer{}
	root := RootCommand(out)

	err := Execute(root, []string{"--help"}, out)
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}

	if !strings.Contains(out.String(), "Usage:") {
		t.Fatalf("expected help output, got %q", out.String())
	}
}
