from SCons.Script import Import

Import("env")

# Set ccache directory in project
env['ENV']['CCACHE_DIR'] = str(env.subst("$PROJECT_DIR/.ccache"))

# Configure ccache settings
env['ENV']['CCACHE_COMPRESS'] = "1"
env['ENV']['CCACHE_COMPRESSLEVEL'] = "6"
env['ENV']['CCACHE_MAXSIZE'] = "500M"

# Prepend ccache to compilers
env['CC'] = f"ccache {env['CC']}"
env['CXX'] = f"ccache {env['CXX']}"

# Print ccache statistics
try:
    import subprocess
    print("\nCCache Statistics:")
    subprocess.run(['ccache', '-s'], check=True)
except Exception as e:
    print(f"\nWarning: Could not get ccache statistics: {e}")