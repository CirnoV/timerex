# vim: set sts=4 ts=8 sw=4 tw=99 et:
import sys
API_VERSION = '2.2'

try:
    from ambuild2 import run
    if not run.HasAPI(API_VERSION):
        raise Exception()
except:
    sys.stderr.write(
        'AMBuild {0} must be installed to build this project.\n'.format(API_VERSION))
    sys.stderr.write('http://www.alliedmods.net/ambuild\n')
    sys.exit(1)

builder = run.BuildParser(sourcePath=sys.path[0], api=API_VERSION)
builder.options.add_argument('--core-path', type=str, dest='core_path', default=None,
                             help='Core library path')
builder.options.add_argument('--mms-path', type=str, dest='mms_path', default=None,
                             help='Path to Metamod:Source')
builder.options.add_argument('--sm-path', type=str, dest='sm_path', default=None,
                             help='Path to SourceMod')
builder.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug',
                             help='Enable debugging symbols')
builder.options.add_argument('--enable-optimize', action='store_const', const='1', dest='opt',
                             help='Enable optimization')
builder.options.add_argument('--targets', type=str, dest='targets', default=None,
		                      help="Override the target architecture (use commas to separate multiple targets).")

builder.Configure()
