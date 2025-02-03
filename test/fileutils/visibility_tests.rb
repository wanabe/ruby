# frozen_string_literal: true
require 'test/unit'
require 'fileutils'

##
# These tests are reused in the FileUtils::Verbose, FileUtils::NoWrite and
# FileUtils::DryRun tests

module TestFileUtilsIncVisibility

  FileUtils::METHODS.each do |m|
    eval <<~EOS
      def test_singleton_visibility_#{m}
        assert @fu_module.respond_to?(:#{m}, true),
              "FileUtils::Verbose.#{m} is not defined"
        assert @fu_module.respond_to?(:#{m}, false),
              "FileUtils::Verbose.#{m} is not public"
      end

      def test_visibility_#{m}
        assert respond_to?(:#{m}, true),
              "FileUtils::Verbose\##{m} is not defined"
        assert @fu_module.private_method_defined?(:#{m}),
              "FileUtils::Verbose\##{m} is not private"
      end
    EOS
  end

  FileUtils::StreamUtils_.private_instance_methods.each do |m|
    eval <<~EOS
      def test_singleton_visibility_#{m}
        assert @fu_module.respond_to?(:#{m}, true),
              "FileUtils::Verbose\##{m} is not defined"
      end

      def test_visibility_#{m}
        assert respond_to?(:#{m}, true),
              "FileUtils::Verbose\##{m} is not defined"
      end
    EOS
  end

end
