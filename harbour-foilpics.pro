TEMPLATE = subdirs
SUBDIRS = app harbour-lib

app.file = app.pro
app.depends = harbour-lib-target

harbour-lib.target = harbour-lib-target

OTHER_FILES += README.md LICENSE rpm/*.spec
