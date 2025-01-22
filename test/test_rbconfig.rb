# frozen_string_literal: false
require 'test/unit'
require 'rbconfig'
require 'shellwords'

class TestRbConfig < Test::Unit::TestCase
  WITH_CONFIG = {}

  Shellwords::shellwords(RbConfig::CONFIG["configure_args"]).grep(/\A--with-([^=]*)=(.*)/) do
    WITH_CONFIG[$1.tr('_', '-')] = $2
  end
  Ractor.make_shareable(WITH_CONFIG)

  def test_sitedirs
    RbConfig.makefile_config.each do |key, val|
      next unless /\Asite(?!arch)/ =~ key
      next if WITH_CONFIG[key]
      assert_match(/(?:\$\(|\/)site/, val, key)
    end
  end

  def test_vendordirs
    RbConfig.makefile_config.each do |key, val|
      next unless /\Avendor(?!arch)/ =~ key
      next if WITH_CONFIG[key]
      assert_match(/(?:\$\(|\/)vendor/, val, key)
    end
  end

  def test_archdirs
    RbConfig.makefile_config.each do |key, val|
      next unless /\A(?!site|vendor|archdir\z).*arch.*dir\z/ =~ key
      next if WITH_CONFIG[key]
      assert_match(/\$\(arch|\$\(rubyarchprefix\)/, val, key)
    end
  end

  def test_sitearchdirs
    bug7823 = '[ruby-dev:46964] [Bug #7823]'
    RbConfig.makefile_config.each do |key, val|
      next unless /\Asite.*arch.*dir\z/ =~ key
      next if WITH_CONFIG[key]
      assert_match(/\$\(sitearch|\$\(rubysitearchprefix\)/, val, "#{key} #{bug7823}")
    end
  end

  def test_vendorarchdirs
    bug7823 = '[ruby-dev:46964] [Bug #7823]'
    RbConfig.makefile_config.each do |key, val|
      next unless /\Avendor.*arch.*dir\z/ =~ key
      next if WITH_CONFIG[key]
      assert_match(/\$\(sitearch|\$\(rubysitearchprefix\)/, val, "#{key} #{bug7823}")
    end
  end
end
