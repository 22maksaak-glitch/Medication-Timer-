# medication_timer.rb
require 'json'
require 'date'
require 'colorize'
require 'optparse'

DATA_FILE = 'medications.json'

class Medication
  attr_accessor :id, :name, :dosage, :interval_hours, :last_taken, :history

  def initialize(name, dosage, interval_hours, id = 0, last_taken = nil, history = [])
    @id = id
    @name = name
    @dosage = dosage
    @interval_hours = interval_hours
    @last_taken = last_taken || Time.now
    @history = history.map { |t| Time.parse(t) }
  end

  def is_due?
    Time.now >= next_dose
  end

  def next_dose
    @last_taken + interval_hours * 3600
  end

  def take_dose
    now = Time.now
    @last_taken = now
    @history << now
  end

  def to_hash
    {
      id: @id,
      name: @name,
      dosage: @dosage,
      interval_hours: @interval_hours,
      last_taken: @last_taken.iso8601,
      history: @history.map(&:iso8601)
    }
  end

  def self.from_hash(h)
    new(h['name'], h['dosage'], h['interval_hours'], h['id'], h['last_taken'], h['history'])
  end
end

class TimerApp
  attr_reader :medications

  def initialize
    @medications = []
    load
  end

  def load
    return unless File.exist?(DATA_FILE)
    data = JSON.parse(File.read(DATA_FILE))
    @medications = data.map { |h| Medication.from_hash(h) }
  end

  def save
    File.write(DATA_FILE, JSON.pretty_generate(@medications.map(&:to_hash)))
  end

  def add_medication(name, dosage, interval)
    max_id = @medications.map(&:id).max || 0
    med = Medication.new(name, dosage, interval, max_id + 1)
    @medications << med
    save
    puts "💊 Added medication: #{name} #{dosage} every #{interval}h (ID: #{med.id})"
  end

  def list
    if @medications.empty?
      puts "No medications."
      return
    end
    puts "\n📋 Medications:"
    @medications.each do |m|
      due = m.is_due? ? "🔴 DUE".red : "⏳ #{m.next_dose.strftime('%H:%M')}".green
      puts "  [#{m.id}] #{m.name} #{m.dosage} – #{due}"
    end
  end

  def take_dose(id)
    med = @medications.find { |m| m.id == id }
    unless med
      puts "❌ Medication with ID #{id} not found."
      return
    end
    med.take_dose
    save
    puts "✅ Took #{med.name} #{med.dosage} at #{Time.now.strftime('%H:%M')}"
  end

  def check_due
    due = @medications.select(&:is_due?)
    if due.empty?
      puts "✅ No medications due right now."
      return
    end
    puts "\n⏰ Due Medications:"
    due.each do |m|
      overdue = ((Time.now - m.next_dose) / 60).to_i
      puts "  [#{m.id}] #{m.name} #{m.dosage} – overdue by #{overdue} min"
    end
  end

  def history
    if @medications.empty?
      puts "No medications."
      return
    end
    puts "\n📜 History (last 5 doses per medication):"
    @medications.each do |m|
      puts "  #{m.name}:"
      recent = m.history.last(5)
      if recent.empty?
        puts "    (no history)"
      else
        recent.each { |t| puts "    - #{t.strftime('%Y-%m-%d %H:%M')}" }
      end
    end
  end
end

options = {}
$command = ARGV.shift
if $command.nil?
  puts "Usage: medication_timer.rb <command> [options]"
  exit 1
end

app = TimerApp.new

case $command
when "add"
  name, dosage, interval = ARGV.shift(3)
  app.add_medication(name, dosage, interval.to_f)
when "list"
  app.list
when "take"
  id = ARGV.shift.to_i
  app.take_dose(id)
when "check"
  app.check_due
when "history"
  app.history
else
  puts "Unknown command."
end
