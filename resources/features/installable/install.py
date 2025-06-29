import os
import sys
%IF [<|ENV:FEATURES|> CONTAINS <metadata>]:
import configparser
%END

%IF [<|ENV:FEATURES|> CONTAINS <metadata>]:
def configure_desktop_metadata(prefix):
    meta = configparser.ConfigParser()
    meta.optionxform = str

    meta['Desktop Entry'] = {
        'Version': '1.0',
        'Type': 'Application',
        'Name': '!PROJECT!',
        # 'Comment': '...',
        'Exec': f'{prefix}/bin/!PROJECT!',
        # 'Icon': '...',
        'Terminal': 'false',
        'Categories': 'Utility;'
    }

    return meta

def install_desktop_metadata(prefix):
    meta = configure_desktop_metadata(prefix)
    file = open(f'{prefix}/share/applications/!PROJECT!.desktop', 'w+')
    meta.write(file)
    file.close()
%END

def main(arguments):
    prefix = "~/.local"

    if len(arguments):
        prefix = arguments[0]

    os.system(f'cmake --install build --prefix {prefix}')
%IF [<|ENV:FEATURES|> CONTAINS <metadata>]:
    install_desktop_metadata(os.path.expanduser(prefix))
%END

if __name__ == "__main__":
    sys.argv.pop(0)
    main(sys.argv)
