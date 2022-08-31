package main

import "github.com/pterm/pterm"

type Table struct {
	title  string
	header []string
	rows   [][]string
}

func (t *Table) SetTitle(title string) *Table {
	t.title = title
	return t
}

func (t *Table) SetHeader(fields ...string) *Table {
	t.header = fields
	return t
}

func (t *Table) AddRow(fields ...string) *Table {
	t.rows = append(t.rows, fields)
	return t
}

func (t *Table) Println() {
	pTable := pterm.DefaultTable

	data := t.rows

	if t.header != nil {
		pTable = *pTable.WithHasHeader()
		data = append([][]string{t.header}, data...)
	}

	if t.title != "" {
		pterm.Println(t.title)
	}

	pTable.WithData(data).Render()
}
