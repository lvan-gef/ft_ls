from pathlib import Path

def create_test_folders(path: Path) -> list[Path]:
    paths = ['simple', 'one_qoute', 'one_qoute_beginning', 'mixed_qoutes']
    files = ['abc.txt', 'def.txt', 'ghi.txt', 'jkl.txt',
             'a bc.txt', 'd"ef.txt', 'gh_i.txt', "jk'l.txt"]

    fullpaths = []
    for index, p in enumerate(paths[:1]):
        new_path = path.joinpath(p).absolute()
        new_path.mkdir(parents=True, exist_ok=True)
        fullpaths.append(new_path)
        for f in files:
            file_path = new_path.joinpath(f)
            file_path.touch()

    return fullpaths
