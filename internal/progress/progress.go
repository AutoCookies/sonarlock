package progress

import (
	"encoding/json"
	"fmt"
	"io"
	"time"
)

// Snapshot contains a point-in-time transfer metrics view.
type Snapshot struct {
	BytesTransferred int64
	BytesTotal       int64
	Elapsed          time.Duration
	InstantBPS       float64
	AverageBPS       float64
	ETA              time.Duration
}

// Tracker computes transfer speed and ETA from cumulative byte updates.
type Tracker struct {
	total     int64
	start     time.Time
	last      time.Time
	lastBytes int64
	current   int64
}

// NewTracker constructs a progress tracker for a known byte total.
func NewTracker(total int64) *Tracker {
	now := time.Now()
	return &Tracker{total: total, start: now, last: now}
}

// Add records n additional bytes and returns a new metrics snapshot.
func (t *Tracker) Add(n int) Snapshot {
	now := time.Now()
	t.current += int64(n)

	deltaDur := now.Sub(t.last)
	deltaBytes := t.current - t.lastBytes
	instant := 0.0
	if deltaDur > 0 {
		instant = float64(deltaBytes) / deltaDur.Seconds()
	}

	elapsed := now.Sub(t.start)
	avg := 0.0
	if elapsed > 0 {
		avg = float64(t.current) / elapsed.Seconds()
	}

	eta := time.Duration(0)
	remaining := t.total - t.current
	if avg > 0 && remaining > 0 {
		eta = time.Duration((float64(remaining) / avg) * float64(time.Second))
	}

	t.last = now
	t.lastBytes = t.current

	return Snapshot{
		BytesTransferred: t.current,
		BytesTotal:       t.total,
		Elapsed:          elapsed,
		InstantBPS:       instant,
		AverageBPS:       avg,
		ETA:              eta,
	}
}

// ProgressPrinter writes throttled transfer progress updates.
type ProgressPrinter struct {
	out      io.Writer
	prefix   string
	json     bool
	interval time.Duration
	last     time.Time
}

// NewPrinter creates a progress printer with sane defaults.
func NewPrinter(out io.Writer, prefix string, json bool) *ProgressPrinter {
	return &ProgressPrinter{out: out, prefix: prefix, json: json, interval: 150 * time.Millisecond}
}

// PrintMaybe emits a progress update if throttle timing allows.
func (p *ProgressPrinter) PrintMaybe(s Snapshot) error {
	now := time.Now()
	if !p.last.IsZero() && now.Sub(p.last) < p.interval {
		return nil
	}
	p.last = now
	return p.print(s, false)
}

// PrintFinal emits a final progress summary line.
func (p *ProgressPrinter) PrintFinal(s Snapshot, outputPath string) error {
	return p.print(s, true, outputPath)
}

func (p *ProgressPrinter) print(s Snapshot, final bool, extra ...string) error {
	if p.json {
		rec := map[string]any{
			"phase":            p.prefix,
			"bytes":            s.BytesTransferred,
			"total":            s.BytesTotal,
			"elapsed_seconds":  s.Elapsed.Seconds(),
			"instant_bps":      s.InstantBPS,
			"average_bps":      s.AverageBPS,
			"eta_seconds":      s.ETA.Seconds(),
			"final":            final,
			"throughput_mib_s": s.AverageBPS / (1024 * 1024),
		}
		if len(extra) > 0 {
			rec["output"] = extra[0]
		}
		enc := json.NewEncoder(p.out)
		if err := enc.Encode(rec); err != nil {
			return fmt.Errorf("encode progress JSON: %w", err)
		}
		return nil
	}

	eta := "--"
	if s.ETA > 0 {
		eta = s.ETA.Round(time.Second).String()
	}
	line := fmt.Sprintf("%s bytes=%d/%d inst=%.2f MiB/s avg=%.2f MiB/s eta=%s",
		p.prefix,
		s.BytesTransferred,
		s.BytesTotal,
		s.InstantBPS/(1024*1024),
		s.AverageBPS/(1024*1024),
		eta,
	)
	if final && len(extra) > 0 {
		line += fmt.Sprintf(" output=%s", extra[0])
	}
	if _, err := fmt.Fprintln(p.out, line); err != nil {
		return fmt.Errorf("write progress line: %w", err)
	}
	return nil
}
