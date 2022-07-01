#include "PCH.hpp"
#include "MeshTask.hpp"
#include "QmshTmpLoader.h"
#include "Qmsh.h"
#include <conio.h>
#include <Windows.h>
#include <locale>

const char* CONVERTER_VERSION = "22.3";

bool anyWarning;

//=================================================================================================
// Przygotuj parametry do konwersji
//=================================================================================================
int ConvertToQmsh(const string& filename, const string& output, bool exportPhy, bool allowDoubles)
{
	ConversionData cs;
	cs.input = filename;
	cs.exportPhy = exportPhy;
	cs.allowDoubles = allowDoubles;

	if(output.empty())
	{
		string::size_type pos = cs.input.find_last_of('.');
		if(pos == string::npos)
		{
			if(cs.exportPhy)
				cs.output = cs.input + ".phy";
			else
				cs.output = cs.input + ".qmsh";
		}
		else
			cs.output = cs.input.substr(0, pos);
	}
	else
		cs.output = output;

	try
	{
		Convert(cs);

		Info("Ok.");
		return anyWarning ? 1 : 0;
	}
	catch(cstring err)
	{
		Error("B³¹d: %s", err);
		return 2;
	}
}

//=================================================================================================
// Pocz¹tek programu
//=================================================================================================
int main(int argc, char **argv)
{
	Logger::SetInstance(new ConsoleLogger);

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	if(argc == 1)
	{
		printf("QMSH converter, version %s (output %d).\nUsage: \"converter FILE.qmsh.tmp\". Use \"converter -h\" for list of commands.\n",
			CONVERTER_VERSION, QMSH::VERSION);
		return 0;
	}

	string output;
	int result = 0;
	bool check_subdir = true, force_update = false, exportPhy = false, allowDoubles = false;

	for(int i = 1; i < argc; ++i)
	{
		char* cstr = argv[i];

		if(cstr[0] == '-')
		{
			string str(cstr);

			if(str == "-h" || str == "-help" || str == "-?")
			{
				printf("Converter switches:\n"
					"-h/help/? - list of commands\n"
					"-v - show converter version and input file versions\n"
					"-o FILE - output file name\n"
					"-phy - export only physic mesh (default extension .phy)\n"
					"-normal - export normal mesh\n"
					"-allowdoubles - don't merge vertices with same position/uv/normal\n"
					"-info FILE - show information about mesh (version etc)\n"
					"-infodir DIR - show information about all meshes\n"
					"-details OPTIONS FILE - like info but more details\n"
					"-compare FILE FILE2 - compare two meshes and show differences\n"
					"-upgrade FILE - upgrade mesh to newest version\n"
					"-upgradedir DIR - upgrade all meshes in directory and subdirectories\n"
					"-subdir - check subdirectories in upgradedir (default)\n"
					"-nosubdir - don't check subdirectories in upgradedir\n"
					"-force - force upgrade operation\n"
					"-noforce - don't force upgrade operation (default)\n"
					"Parameters without '-' are treated as input file.\n");
			}
			else if(str == "-v")
			{
				printf("Converter version %s\nHandled input file version: %d..%d\nOutput file version: %d\n",
					CONVERTER_VERSION, QmshTmpLoader::QMSH_TMP_HANDLED_VERSION.x, QmshTmpLoader::QMSH_TMP_HANDLED_VERSION.y, QMSH::VERSION);
			}
			else if(str == "-o")
			{
				if(i + 1 < argc)
				{
					++i;
					output = argv[i];
				}
				else
				{
					Warn("Missing OUTPUT PATH for '-o'!");
					anyWarning = true;
				}
			}
			else if(str == "-phy")
				exportPhy = true;
			else if(str == "-normal")
				exportPhy = false;
			else if(str == "-info")
			{
				if(i + 1 < argc)
				{
					++i;
					MeshInfo(argv[i]);
				}
				else
				{
					Warn("Missing FILE for '-info'!");
					anyWarning = true;
				}
			}
			else if(str == "-infodir")
			{
				if(i + 1 < argc)
				{
					++i;
					MeshInfoDir(argv[i], check_subdir);
				}
				else
				{
					Warn("Missing DIR for '-infodir'!");
					anyWarning = true;
				}
			}
			else if(str == "-details")
			{
				if(i + 2 < argc)
				{
					MeshInfo(argv[i + 2], argv[i + 1]);
					i += 2;
				}
				else
				{
					Warn("Missing OPTIONS or FILE for '-details'!");
					anyWarning = true;
					++i;
				}
			}
			else if(str == "-compare")
			{
				if(i + 2 < argc)
				{
					Compare(argv[i + 1], argv[i + 2]);
					i += 2;
				}
				else
				{
					Warn("Missing FILEs for '-compare'!");
					anyWarning = true;
					++i;
				}
			}
			else if(str == "-upgrade")
			{
				if(i + 1 < argc)
				{
					++i;
					Upgrade(argv[i], force_update);
				}
				else
				{
					Warn("Missing FILE for '-upgrade'!");
					anyWarning = true;
				}
			}
			else if(str == "-upgradedir")
			{
				if(i + 1 < argc)
				{
					++i;
					UpgradeDir(argv[i], force_update, check_subdir);
				}
				else
				{
					Warn("Missing DIR for '-upgradedir'!");
					anyWarning = true;
				}
			}
			else if(str == "-subdir")
				check_subdir = true;
			else if(str == "-nosubdir")
				check_subdir = false;
			else if(str == "-force")
				force_update = true;
			else if(str == "-noforce")
				force_update = false;
			else if(str == "-allowdoubles")
				allowDoubles = true;
			else
			{
				Warn("Unknown switch \"%s\"!\n", cstr);
				anyWarning = true;
			}
		}
		else
		{
			string tstr(cstr);
			result = Max(ConvertToQmsh(tstr, output, exportPhy, allowDoubles), result);
			output.clear();
		}
	}

	if(IsDebuggerPresent())
	{
		printf("Press any key to exit...");
		_getch();
	}

	if(anyWarning && result == 0)
		result = 1;

	return result;
}
