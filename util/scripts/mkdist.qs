/************************************************************
 * Some global variables needed throughout the packaging script.
 */
const qtDir = System.getenv("QTDIR");
const qmakeCommand = qtDir + "/bin/qmake";

const qdocDir = qtDir + "/util/qdoc";
const qdocCommand = qdocDir + "/qdoc";

const qpkgDir = qtDir + "/util/install/package"
const qpkgCommand = qtDir + "/bin/package"

const outputDir = System.getenv("PWD");

const validPlatforms = ["win", "x11", "mac", "embedded"];
const validEditions = ["free", "commercial"];
const validSwitches = ["branch", "version"];

const user = System.getenv("USER");

var options = [];	// list of all package options
var tmpDir;		// directory for all intermediate files
var distDir;		// directory for P4 checkout
var p4Port;
var p4Command;

/*******************************************************************************
 * Here we go
 */

System.println("Initializing...");
parseArgc();
initialize();
System.println("Checking tools...");
checkTools();
System.println("Building qdoc...");
buildQdoc();
// System.println("Building qpkg...");
// buildQpkg();
System.println("Checkin out from P4...");
checkout();
var start = Date().getTime();
var list = getFileList(distDir);
var end = Date().getTime();
System.println("getFileList(%1) took: %2 milliseconds count: %3".arg(qtDir).arg(end-start).arg(list.length));
// cleanup();

/************************************************************
 * Parses and checks the commandline options and puts them into options[key] = value
 */
function parseArgc()
{
    validOptions = validPlatforms.toString() + validEditions.toString() + validSwitches.toString();
    for (var i=0; i<argc.length; ++i) {
	if (argc[i].startsWith("-do")) {
	    optionKey = argc[i].split("-")[2];
	    optionValue = true;
	} else if (argc[i].startsWith("-no")) {
	    optionKey = argc[i].split("-")[2];
	    optionValue = false;
	} else if (argc[i].startsWith("-")) {
	    optionKey = argc[i].split("-")[1];
	    optionValue = argc[++i];
	} else {
	    throw "Invalid option format: %1".arg(argc[i]);
	}

	// check that the optionKey is valid
	if (validOptions.find(optionKey) == -1)
	    throw "Unknown option: %1".arg(optionKey);
	else
	    options[optionKey] = optionValue;
    }
}

/************************************************************
 * Setup directories and query environment variables
 */
function initialize()
{
    // checks that branch and version has been specified
    if (options["branch"] == undefined)
	throw "branch not specified.";
    if (options["version"] == undefined)
	throw "version not specified.";


    // by default turn on all valid platforms were not defined
    for (var i in validPlatforms)
	if (!(validPlatforms[i] in options))
	    options[validPlatforms[i]] = true;

    // by default turn on all valid editions that were not defined
    for (var i in validEditions)
	if (!(validEditions[i] in options))
	    options[validEditions[i]] = true;

    // finds a tmpDir
    if (tmpDir == undefined || !File.exists(tmpDir)) {
	if (File.exists(System.getenv("HOME") + "/tmp"))
	    tmpDir = System.getenv("HOME") + "/tmp";
	else if (File.exists("/tmp"))
	    tmpDir = "/tmp";
	else
	    throw "Unable to find tmp directory";
    }
    // creates distDir
    distDir = tmpDir + "/qt-" + options["branch"] + "-" + user + "-" + Date().getTime();
    var dir = new Dir(distDir);
    if (dir.exists)
	dir.rmdirs();
    dir.mkdir();

    // setting up p4
    if (p4Port == undefined)
	p4Port = "p4.troll.no:866";
    if (p4Command == undefined || !File.exists(p4Command))
	p4Command = System.getenv("which p4");
    if (!File.exists(p4Command))
	p4Command = "/usr/local/bin/p4";

//     for (var i in options)
// 	System.println("options[%1] = %2".arg(i).arg(options[i]));
}

/************************************************************
 * Verify that the necesary tools are available.
 */
function checkTools()
{
    try {
	Process.execute([qmakeCommand, "-help"]);
	Process.execute("zip -help");
	Process.execute("tar --help");
	Process.execute("gzip -h");
	Process.execute(p4Command);
    } catch (e) {
	throw "Tool failed: %1".arg(e);
    }
}

/************************************************************
 * Builds and checks qdoc
 */
function buildQdoc()
{
    var dir = new Dir(qdocDir);
    dir.setCurrent();
    Process.execute("%1 qdoc.pro".arg(qmakeCommand));
    if (Process.execute("make") != 0)
	throw "Failed to build qdoc:\n %1".arg(Process.stderr);
    // test qdoc
    Process.execute( [qdocCommand, "-help"] );
}

/************************************************************
 * Builds and checks qpkg
 */
function buildQpkg()
{
    var dir = new Dir(qpkgDir);
    dir.setCurrent();
    Process.execute("%1 package.pro".arg(qmakeCommand));
    if (Process.execute("make") != 0)
	throw "Failed to build qpkg:\n %1".arg(Process.stderr);
    // test qpkg
    Process.execute( [qpkgCommand, "-help"] );
}

/************************************************************
 * checkouts from P4 and puts everything in distDir
 */
function checkout()
{
    // check that the branch exist
    var branchPath = "//depot/qt/" + options["branch"];
    Process.execute([p4Command, "fstat", branchPath + "/configure"]);
    if (Process.stdout.find("depotFile") == -1)
	throw "Branch: " + branchPath + " does not exist.";
    
    // check that the label exists
    var label = "qt/" + options["version"];
    Process.execute([p4Command, "labels", branchPath + "/configure"]);
    if (Process.stdout.find("Label " + label + " ") == -1)
	throw "Label: " + label + " does not exist, or not in this branch.";

    // generate clientSpec
    var tmpClient="qt-release-tmp-" + user;
    Process.execute([p4Command, "client", "-t", "qt-release-3x", "-o", tmpClient]);
    var clientSpec = Process.stdout.split("\n");
    for (var i in clientSpec) {
	clientSpec[i] = clientSpec[i].replace(/^Root:.*/, "Root: " + distDir);
	clientSpec[i] = clientSpec[i].replace(/X.Y/, options["branch"]);
	clientSpec[i] = clientSpec[i].replace(/\bnomodtime\b/, "modtime");
    }
    // save it
    clientSpec = clientSpec.join("\n");
    Process.execute([p4Command, "client", "-i"], clientSpec);

    // checkout
    Process.execute([p4Command, "-c", tmpClient, "-d", distDir, "sync", "-f", "...@" + label]);
}


/************************************************************
 * iterates over the fileList and removes any files found in the
 * remove patterns and keeps any files found in the keep pattern, any
 * file not found in any of the patterns throws an exception
 */
function purgeFiles(fileList, remove, keep)
{
    var doRemove = false;
    var doKeep = false;
    for (var i in fileList) {
	// check if the file should be removed
	for (var r in remove) {
	    if (fileList[i].find(remove[r]) != -1) {
		doRemove = true;
		break;
	    }
	}

	// remove file
	if (doRemove) {
	    if (File.exists[fileList[i]])
		File.remove(fileList[i]);
	    continue;
	}

	// check if the file should be kept
	for (var k in keep) {
	    if (fileList[i].find(keep[k]) != -1) {
		doKeep = true;
		break;
	    }
	}

	// bail out
	if (!doKeep)
	    throw "File: %1 not found in remove nor keep filter, bailing out.";
    }
}

/************************************************************
 * gets a list of all files and subdirectories from the specified directory
 */
function getFileList(dir)
{
    var dir = new Dir(dir);
    dir.setCurrent();
    var result = new Array();

    // add files expanded to absolutepath to result
    var files = dir.entryList("*", Dir.Files);
    for (var f in files)
	result.push(dir.absFilePath(files[f]));

    // expand dirs to absolute path
    var dirs = new Array();
    var tempDirs = dir.entryList("*", Dir.Dirs);
    for (var t in tempDirs) {
	if (tempDirs[t] != "." && tempDirs[t] != "..")
	    dirs.push(dir.absFilePath(tempDirs[t]));
    }
    
    for (var i=0; i<dirs.length; ++i) {
 	// cd to directory and add directory to result
 	dir.cd(dirs[i]);
 	result.push(dirs[i]);

	// add files expanded to absolutepath to result
	var files = dir.entryList("*", Dir.Files);
	for (var f in files)
	    result.push(dir.absFilePath(files[f]));

	// adds subDirs to dirs
	tempDirs = dir.entryList("*", Dir.Dirs);
	for (var t in tempDirs) {
	    if (tempDirs[t] != "." && tempDirs[t] != "..")
		dirs.push(dir.absFilePath(tempDirs[t]));
	}
    }
    return result;
}

/************************************************************
 * cleans up temp files
 */
function cleanup()
{
    // deletes distDir
    var dir = new Dir(distDir);
    if (dir.exists)
	dir.rmdirs();
}
