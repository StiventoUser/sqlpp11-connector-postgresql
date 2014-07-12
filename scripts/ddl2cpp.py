#!/usr/bin/python
#
# Generate C++ structs for the tables.

import os
import sys
import psycopg2
import argparse

# Generate a argument parser
parser = argparse.ArgumentParser(description='Create C++ structs from a database table structure.')
parser.add_argument('-u', '--user', dest='user', required=True, help='PostgreSQL user')
parser.add_argument('--host', dest='host', required=True, help='PostgreSQL host')
parser.add_argument('-p', '--password', dest='password', required=True, help='PostgreSQL password')
parser.add_argument('-d', '--dbname', dest='dbname', required=True, help='PostgreSQL database')
parser.add_argument('-o', '--output-dir', dest='outputdir', help='Output directory', default='tables/')
parser.add_argument('-n', '--namespace', dest='namespace', help='C++ namespace', default='model')
args = parser.parse_args()

def _writeLine(fd, indent, line):
    fd.write(("\t" * indent) + line + "\n")

def _getIncludeGuard(namespace, table):
    return namespace.upper() + "_" + table.upper() + "_H"

# SQL types
types = {
    'tinyint': 'tinyint',
    'smallint': 'smallint',
    'integer': 'integer',
    'int': 'integer',
    'bigint': 'bigint',
    'char': 'char_',
    'varchar': 'varchar',
    'character varying': 'varchar',
    'text': 'text',
    'bool': 'boolean',
    'double': 'floating_point',
    'float': 'floating_point',

    # For now keep this a varchar
    'time without time zone': 'varchar',
    'timestamp without time zone': 'varchar',
}

# Connect to the database and fetch information from the information_schema
# schema
conn = psycopg2.connect("host=" + args.host + " user=" + args.user + " password=" + args.password + " dbname=" + args.dbname)
#conn = psycopg2.connect("host=192.168.5.10 user=moneymaker password=0AqY4Xan4FK7qszM dbname=moneymaker")
curs = conn.cursor()

# First fetch all tables
curs.execute("""SELECT table_name FROM information_schema.tables WHERE table_schema = 'public'""")
tables = curs.fetchall()
for table in tables:
    print "Table: " + table[0]
    fd = open(os.path.join(args.outputdir, table[0] + '.h'), 'w')
    _writeLine(fd, 0, "#ifndef " + _getIncludeGuard(args.namespace, table[0]))
    _writeLine(fd, 0, "#define " + _getIncludeGuard(args.namespace, table[0]))
    _writeLine(fd, 0, "")
    _writeLine(fd, 0, "#include <sqlpp11/table.h>")
    _writeLine(fd, 0, "#include <sqlpp11/column_types.h>")
    _writeLine(fd, 0, "")
    _writeLine(fd, 0, "namespace model {")
    _writeLine(fd, 0, "")
    _writeLine(fd, 1, "namespace " + table[0] + "_ {")
    _writeLine(fd, 0, "")

    # Fetch all columns for this table
    curs.execute("""SELECT * FROM information_schema.columns WHERE table_schema = 'public' AND table_name = '%s' ORDER BY table_name ASC, ordinal_position ASC""" % (table[0],))
    columns = curs.fetchall()
    for column in columns:
        print "Column: " + column[3] + ", type: " + column[7] + " null: " + column[6]
        _writeLine(fd, 2, "struct " + column[3].capitalize() + " {")
        _writeLine(fd, 3, "struct _name_t {")
        _writeLine(fd, 4, "static constexpr const char *_get_name() { return \"" + column[3] + "\"; }")
        _writeLine(fd, 4, "template<typename T>")
        _writeLine(fd, 5, "struct _member_t {")
        _writeLine(fd, 6, "T " + column[3] + ";")
        _writeLine(fd, 6, "T &operator()() { return " + column[3] + "; }")
        _writeLine(fd, 6, "const T &operator()() const { return " + column[3] + "; }")
        _writeLine(fd, 5, "};")
        _writeLine(fd, 3, "};")
        _writeLine(fd, 3, "using _value_type = sqlpp::" + types[column[7]] + ";")
        _writeLine(fd, 3, "struct _column_type {");

        # Default value
        if column[5]:
            _writeLine(fd, 4, "using _must_not_insert = std::true_type;")
            _writeLine(fd, 4, "using _must_not_update = std::true_type;")

        # Is column not null and no default value?
        if column[6] == "NO" and not column[5]:
            _writeLine(fd, 4, "using _require_insert = std::true_type;")

        # Field can be NULL
        if column[6] == "YES":
            _writeLine(fd, 4, "using _can_be_null = std::true_type;")

        _writeLine(fd, 3, "};")
        _writeLine(fd, 2, "};")

    _writeLine(fd, 1, "}")
    _writeLine(fd, 0, "")

    # Columns are now written as C++ structs, now use them in the definition of
    # the table.
    _writeLine(fd, 1, "struct " + table[0] + " : sqlpp::table_t<" + table[0] + ",")
    if len(columns) == 1:
        _writeLine(fd, 4, table[0] + "_::" + columns[0][3].capitalize() + "> {")
    else:
        for column in columns[:-1]:
            _writeLine(fd, 4, table[0] + "_::" + column[3].capitalize() + ",")
        _writeLine(fd, 4, table[0] + "_::" + columns[-1][3].capitalize() + "> {")

    _writeLine(fd, 2, "using _value_type = sqlpp::no_value_t;")
    _writeLine(fd, 2, "struct _name_t {")
    _writeLine(fd, 3, "static constexpr const char *_get_name() { return " + table[0] + "; }")
    _writeLine(fd, 3, "template<typename T>")
    _writeLine(fd, 4, "struct _member_t {")
    _writeLine(fd, 5, "T " + table[0] + ";")
    _writeLine(fd, 5, "T &operator()() { return " + table[0] + "; }");
    _writeLine(fd, 5, "const T &operator()() const { return " + table[0] + "; }");
    _writeLine(fd, 4, "};")
    _writeLine(fd, 2, "};")

    _writeLine(fd, 1, "};")

    # end of namespace
    _writeLine(fd, 0, "}")
    _writeLine(fd, 0, "")
    _writeLine(fd, 0, "#endif")

    # Close file
    fd.close()

conn.close()
