"""
Simple validation and test module
Verifies the smart grid system is properly configured
"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def check_module_structure():
    """Check if all required modules are present"""
    
    modules_to_check = [
        'common.utils',
        'common.encryption',
        'smart_meters.meter_client',
        'analytics_server.server',
        'logger.performance_logger'
    ]
    
    print("Checking module structure...")
    
    for module_name in modules_to_check:
        try:
            __import__(module_name)
            print(f"  ✓ {module_name}")
        except ImportError as e:
            print(f"  ✗ {module_name}: {e}")
            return False
    
    return True


def check_configuration():
    """Check if configuration file exists and is loadable"""
    
    print("\nChecking configuration...")
    
    try:
        from common.utils import ConfigurationManager
        config = ConfigurationManager.load_config('./config/system_config.yaml')
        
        required_sections = ['ckks', 'smart_meters', 'analytics_server', 'logger']
        
        for section in required_sections:
            if section in config:
                print(f"  ✓ {section}")
            else:
                print(f"  ✗ {section} missing from config")
                return False
        
        return True
        
    except Exception as e:
        print(f"  ✗ Configuration error: {e}")
        return False


def check_dependencies():
    """Check if required packages are installed"""
    
    print("\nChecking dependencies...")
    
    dependencies = {
        'yaml': 'PyYAML',
        'numpy': 'NumPy',
    }
    
    optional_deps = {
        'tenseal': 'TensorSEAL (homomorphic encryption)',
    }
    
    all_ok = True
    
    for module, name in dependencies.items():
        try:
            __import__(module)
            print(f"  ✓ {name}")
        except ImportError:
            print(f"  ✗ {name} (required)")
            all_ok = False
    
    print("\nOptional dependencies:")
    for module, name in optional_deps.items():
        try:
            __import__(module)
            print(f"  ✓ {name}")
        except ImportError:
            print(f"  ⚠ {name} (install with: pip install {module})")
    
    return all_ok


def check_file_structure():
    """Check if all required files exist"""
    
    print("\nChecking file structure...")
    
    required_files = [
        'config/system_config.yaml',
        'common/__init__.py',
        'common/utils.py',
        'common/encryption.py',
        'smart_meters/__init__.py',
        'smart_meters/meter_client.py',
        'analytics_server/__init__.py',
        'analytics_server/server.py',
        'logger/__init__.py',
        'logger/performance_logger.py',
        'main.py',
        'run_meters.py',
        'run_server.py',
        'run_benchmark.py',
    ]
    
    all_exist = True
    for file_path in required_files:
        if os.path.exists(file_path):
            print(f"  ✓ {file_path}")
        else:
            print(f"  ✗ {file_path} missing")
            all_exist = False
    
    return all_exist


def run_all_checks():
    """Run all validation checks"""
    
    print("="*60)
    print("SMART GRID SYSTEM - VALIDATION CHECK")
    print("="*60)
    
    checks = [
        ("File Structure", check_file_structure),
        ("Dependencies", check_dependencies),
        ("Configuration", check_configuration),
        ("Module Structure", check_module_structure),
    ]
    
    results = []
    for check_name, check_func in checks:
        try:
            result = check_func()
            results.append((check_name, result))
        except Exception as e:
            print(f"\n✗ {check_name} check failed: {e}")
            results.append((check_name, False))
    
    # Summary
    print("\n" + "="*60)
    print("VALIDATION SUMMARY")
    print("="*60)
    
    all_passed = True
    for check_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{check_name:.<40} {status}")
        if not result:
            all_passed = False
    
    print("="*60)
    
    if all_passed:
        print("\n✓ All checks passed! System is ready to use.")
        print("\nQuick start:")
        print("  python main.py --mode demo")
        return 0
    else:
        print("\n✗ Some checks failed. Please install dependencies:")
        print("  pip install -r requirements.txt")
        return 1


if __name__ == '__main__':
    sys.exit(run_all_checks())
