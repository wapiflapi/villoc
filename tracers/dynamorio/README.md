## Requirement
[DynamoRIO](https://github.com/DynamoRIO/dynamorio)

## Build
Set the environment variable DYNAMORIO_HOME to the absolute path of your DynamoRIO installation\
Execute `./build.sh`\
To compile the tracer for a 32bits target on a 64bits os execute `./build.sh 32`

## usage
`$DYNAMORIO_HOME/bin64/drrun -c villoc_tracer -- ./target |& ./villoc.py - out.html;`\
For a 32 bit target on a 64 bit OS:\
`$DYNAMORIO_HOME/bin32/drrun -c villoc_tracer -- ./target |& ./villoc.py - out.html;`
