#include "config.h"
#include "eval.h"
#include "command.h"
#include <stdio.h>
#include <cctype>
#include <glib.h>

static string dequote(const string &line, string::const_iterator &p, bool &completed)
{
	string arg;
	char qt = *p++;

	while(p != line.end() && *p != qt) {
		if(qt == '"' && *p == '\\') {
			if(++p == line.end()) {
				completed = false;
				break;
			}
		}
		arg += *p++;
	}
	if(p == line.end())
		completed = false;
	else
		p++;
	return arg;
}

static vector<string> tokenize(const string &line, bool &completed)
{
	vector<string> args;
	string cur;
	string::const_iterator p = line.begin();

	completed = true;
	while(p != line.end()) {
		cur = "";
		while(isspace(*p))
			p++;
		if(p == line.end())
			break;
		while(p != line.end() && !isspace(*p)) {
			if(*p == '\\') {
				if(++p == line.end()) {
					completed = false;
					break;
				}
				cur += *p++;
			} else if(*p == '"' || *p == '\'')
				cur += dequote(line, p, completed);
			else
				cur += *p++;
		}
		args.push_back(cur);
	}
	return args;
}

int eval_command(const Session &session, char *expr, int& quit, bool interactive)
{
	const Command *command;
	CommandContext context(session);
	bool completed;

    quit = 0;
	context.args = tokenize(expr, completed);
	if(!completed) {
		fprintf(stderr, "Incomplete command.  Multi-line entry of commands not yet implemented.\n");
		return COMERR_SYNTAX;
	}
	if(context.args.size()) {
		if(!(command = command_lookup(context.args[0]))) {
			fprintf(stderr, "Invalid command: %s\n", context.args[0].c_str());
			return COMERR_BADCOMMAND;
		}
		if(!interactive && (command->get_flags() & COMFLAG_INTERACTIVE)) {
			fprintf(stderr, "The `%s' command is available only in interactive mode\n", command->get_primary_name().c_str());
			return COMERR_NOTINTERACTIVE;
		}
		command->execute(context);
		if(context.quit) {
			quit = 1;
        }
		if(context.result_code == COMERR_SYNTAX) {
			fprintf(stderr, "Usage: %s\n", command->get_syntax().c_str());
        }
	}
	return context.result_code;
}

int eval_command_string(const Session& session, char *expr, int& quit, bool interactive)
{
	char **commands;
	int i, result = 0;

	g_strdelimit(expr, "\r\n;", ';');
	commands = g_strsplit(expr, ";", 0);
	for(i = quit = 0; !quit && commands[i]; i++)
		result = eval_command(session, commands[i], quit, interactive);
	g_strfreev(commands);
	return result;
}

